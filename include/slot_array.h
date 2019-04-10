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
    class slot_array
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
        static_assert(N <= invalid_index, "slot_array too large for index_type");

        struct lookup_t
        {
            version_type version;
            index_type next_free;
            index_type data_index;
        };

    public:
        slot_array()
            : m_size()
            , m_free_head()
            , m_data()
            , m_lookups()
            , m_erase()
        {
            reset_metadata();
        }

        ~slot_array()
        {
            destroy_all();
        }

        // This is a big, fixed data structure for holding resources
        slot_array(const slot_array&)            = delete;
        slot_array& operator=(const slot_array&) = delete;
        slot_array(slot_array&&)                 = delete;
        slot_array& operator=(slot_array&&)      = delete;

        // Size and capacity
        constexpr size_t size()        const noexcept { return m_size; }
        constexpr size_t max_size()    const noexcept { return N; }
        constexpr bool empty()         const noexcept { return m_size == 0; }

        // Iterators
        iterator begin()                     noexcept { return m_data.data(); }
        const_iterator begin()         const noexcept { return m_data.data(); }
        const_iterator cbegin()        const noexcept { return m_data.data(); }
        iterator end()                       noexcept { return m_data.data() + m_size; }
        const_iterator end()           const noexcept { return m_data.data() + m_size; }
        const_iterator cend()          const noexcept { return m_data.data() + m_size; }

        /// <summary>
        /// Inserts a value into the slot array.
        /// </summary>
        template<typename ... Args>
        key_type emplace_back(Args&& ... args, meta_type meta_data = 0)
        {
            if (m_free_head == invalid_index)
                throw std::out_of_range("slot_array has no free slots");
            const index_type lookup_index = m_free_head;
            lookup_t& lookup = m_lookups[lookup_index];

            // We could probably recover by just orphaning this slot, but
            // that would make insertion O(n) as we'd have to find the next.
            // Otherwise this is fatal as it makes all key handles unsafe.
            if (increment_version(lookup.version) == false)
                throw std::overflow_error("slot_array version overflow");

            // Store data and lookup
            m_data.emplace(m_size, std::forward<Args>(args) ...);
            m_erase[m_size] = lookup_index;
            lookup.data_index = static_cast<index_type>(m_size);

            // Pop free list and increase size
            m_free_head = lookup.next_free;
            lookup.next_free = invalid_index;
            ++m_size;

            return key_type(lookup.version, lookup_index, meta_data);
        }
        
        /// <summary>
        /// Tries to get a value at the given key.
        /// Will return a nullptr if the key did not match any values.
        /// </summary>
        T* try_get(key_type key)
        {
            if (lookup_t* lookup = resolve_key(key))
                return std::addressof(m_data[lookup->data_index]);
            return nullptr;
        }

        /// <summary>
        /// Tries to get a value at the given key.
        /// Will return a nullptr if the key did not match any values.
        /// </summary>
        const T* try_get(key_type key) const
        {
            if (const lookup_t* lookup = resolve_key(key))
                return std::addressof(m_data[lookup->data_index]);
            return nullptr;
        }

        /// <summary>
        /// Tries to remove a given key. 
        /// Returns false if no value was found.
        /// </summary>
        bool try_remove(key_type key)
        {
            using std::swap;

            lookup_t* lookup = resolve_key(key);
            if (lookup == nullptr)
                return false;
            lookup_t& lookup_cursor = *lookup;

            // Get information for the element we want to remove
            const index_type data_index_cursor = lookup_cursor.data_index;
            const index_type lookup_index_cursor = m_erase[data_index_cursor];

            // Get information for the last element in the array
            const index_type data_index_tail = static_cast<index_type>(m_size - 1);
            const index_type lookup_index_tail = m_erase[data_index_tail];
            lookup_t& lookup_tail = m_lookups[lookup_index_tail];

            // Swap data with the value at the end of our storage and destroy
            // WARNING: These operations may throw exceptions!
            swap(m_data[data_index_cursor], m_data[data_index_tail]);
            m_data.destroy(data_index_tail);

            // Update erase list
            m_erase[data_index_cursor] = m_erase[data_index_tail];
            m_erase[data_index_tail] = invalid_index;

            // Update the two affected lookups
            lookup_cursor.data_index = invalid_index;
            lookup_tail.data_index = data_index_cursor;

            // Update the free list and size
            lookup_cursor.next_free = m_free_head;
            m_free_head = lookup_index_cursor;
            --m_size;

            return true;
        }

        /// <summary>
        /// Clears and reorganizes the slot array.
        /// Does not reset version numbers on slots.
        /// </summary>
        void clear()
        {
            destroy_all();
            reset_metadata();
            m_size = 0;
        }

    private:
        static bool increment_version(uint32_t& version)
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

            for (index_type idx = 0; idx < N; ++idx)
            {
                m_lookups[idx].data_index = invalid_index;
                m_lookups[idx].next_free = (idx + 1);
                m_erase[idx] = invalid_index;
            }

            m_lookups[N - 1].next_free = invalid_index;
            m_free_head = 0;
        }

        lookup_t* resolve_key(key_type key)
        {
            const index_type lookup_index = key.m_index;
            if (evaluate_index(lookup_index) == false)
                return nullptr;

            lookup_t& lookup = m_lookups[lookup_index];
            if (evaluate_lookup(key, lookup) == false)
                return nullptr;

            return std::addressof(lookup);
        }

        const lookup_t* resolve_key(key_type key) const
        {
            const index_type lookup_index = key.m_index;
            if (evaluate_index(lookup_index) == false)
                return nullptr;

            const lookup_t& lookup = m_lookups[lookup_index];
            if (evaluate_lookup(key, lookup) == false)
                return nullptr;

            return std::addressof(lookup);
        }

        bool evaluate_index(index_type lookup_index) const
        {
            if (lookup_index >= N)
                return false; // Out of range
            return true;
        }

        bool evaluate_lookup(key_type key, lookup_t lookup) const
        {
            if (lookup.data_index == invalid_index)
                return false; // Element missing
            if (lookup.data_index >= m_size)
                return false; // Out of range
            if (lookup.version != key.m_version)
                return false; // Key outdated
            return true;
        }

        void destroy_all()
        {
            for (uint16_t idx = 0; idx < m_size; ++idx)
                m_data.destroy(idx);
        }

        size_t                    m_size;
        index_type                m_free_head;
        nonstd::raw_buffer<T, N>  m_data;
        std::array<lookup_t, N>   m_lookups;
        std::array<index_type, N> m_erase;
    };
}
