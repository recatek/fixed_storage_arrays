#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>

#include "raw_buffer.h"
#include "versioned_key.h"

namespace nonstd
{
    template<class T, size_t N, typename Key = versioned_key>
    class keyed_array
    {
    public:
        using value_type        = T;
        using const_value_type  = const T;
        using pointer           = T*;
        using const_pointer     = const T*;
        using reference         = T&;
        using const_reference   = const T&;
        using iterator          = T*;
        using const_iterator    = const T*;
        using key_type          = Key;
        using version_type      = typename key_type::version_type;
        using index_type        = typename key_type::index_type;
        using meta_type         = typename key_type::meta_type;

        static constexpr auto capacity = N;

    private:
        static const index_type max_index = std::numeric_limits<index_type>::max();
        static const index_type invalid_index = max_index;
        static const index_type slot_full = max_index - 1;
        static_assert(N <= slot_full, "keyed_array too large for index_type");

    public:
        keyed_array()
            : m_free_head()
            , m_data()
            , m_versions()
            , m_free()
        {
            reset_metadata();
        }

        ~keyed_array()
        {
            destroy_all();
        }

        // This is a big, fixed data structure for holding resources
        keyed_array(const keyed_array&)            = delete;
        keyed_array& operator=(const keyed_array&) = delete;
        keyed_array(keyed_array&&)                 = delete;
        keyed_array& operator=(keyed_array&&)      = delete;

        // Size and capacity
        constexpr size_t max_size()   const noexcept { return N; }
        constexpr bool full()         const noexcept { return m_free_head == invalid_index; }

        /// <summary>
        /// Inserts a value into the keyed array.
        /// Optionally provide a meta_data value to inscribe into the key.
        /// </summary>
        template<typename ... Args>
        key_type emplace_back(Args&& ... args, meta_type meta = 0)
        {
            if (m_free_head == invalid_index)
                throw std::out_of_range("keyed_array has no free slots");
            const index_type index = m_free_head;

            // We could probably recover by just orphaning this slot, but
            // that would make insertion O(n) as we'd have to find the next.
            // Otherwise this is fatal as it makes all key handles unsafe.
            if (increment_version(m_versions[index]) == false)
                throw std::overflow_error("keyed_array version overflow");

            // Store data and update structure status information
            m_data.emplace(index, std::forward<Args>(args) ...);
            m_free_head = m_free[index];
            m_free[index] = slot_full;

            return key_type(m_versions[index], index, meta);
        }

        /// <summary>
        /// Tries to get a value at the given key.
        /// Will return a nullptr if the key did not match any values.
        /// </summary>
        T* try_get(key_type key)
        {
            if (evaluate_key(key) == false)
                return nullptr;
            return std::addressof(m_data[key.m_index]);
        }

        /// <summary>
        /// Tries to get a value at the given key.
        /// Will return a nullptr if the key did not match any values.
        /// </summary>
        const T* try_get(key_type key) const
        {
            if (evaluate_key(key) == false)
                return nullptr;
            return std::addressof(m_data[key.m_index]);
        }

        /// <summary>
        /// Tries to remove a given key. 
        /// Returns false if no value was found.
        /// </summary>
        bool try_remove(key_type key)
        {
            if (evaluate_key(key) == false)
                return false;

            destroy_at(key.m_index);
            return true;
        }

        /// <summary>
        /// Clears and reorganizes the keyed array.
        /// Does not reset version numbers on slots.
        /// </summary>
        void clear()
        {
            destroy_all();
            reset_metadata();
        }

    private:
        static bool increment_version(version_type& version)
        {
            return (++version > 0);
        }

        void reset_metadata()
        {
            if constexpr (N == 0)
            {
                m_free_head = invalid_index;
                return;
            }

            for (index_type pos = 0; pos < (N - 1); ++pos)
            {
                m_free[pos] = (pos + 1);
            }

            m_free[N - 1] = invalid_index;
            m_free_head = 0;
        }

        void destroy_at(index_type index)
        {
            m_data.destroy(index);
            m_free[index] = m_free_head;
            m_free_head = index;
        }

        bool evaluate_key(key_type key) const
        {
            const index_type index = key.m_index;
            if (index >= N)
                return false; // Out of range
            if (m_free[index] != slot_full)
                return false; // Element missing
            if (key.m_version != m_versions[index])
                return false; // Key outdated
            return true;
        }

        void destroy_all()
        {
            for (index_type idx = 0; idx < N; ++idx)
                if (m_free[idx] == slot_full)
                    m_data.destroy(idx);
        }

        index_type                  m_free_head;
        nonstd::raw_buffer<T,  N>   m_data;
        std::array<version_type, N> m_versions;
        std::array<index_type, N>   m_free;
    };
}
