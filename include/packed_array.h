#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>

#include "raw_buffer.h"

namespace nonstd
{
    /// <summary>
    /// An ordered and packed resource array for large resources.
    /// Is neither copyable nor movable, but does allow forwarding on
    /// object storage, and does not require or waste default initialization.
    /// Objects cannot be individually removed once added, only a full clear.
    /// </summary>
    template<class T, size_t N, typename Key = size_t>
    class packed_array
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
        using index_type        = Key;

        static constexpr auto capacity = N;

    private:
        static const index_type max_index = std::numeric_limits<index_type>::max();
        static const index_type invalid_index = max_index;
        static_assert(N <= invalid_index, "packed_array too large for key");

    public:
        packed_array()
            : m_size()
            , m_data()
        {
            // Pass
        }

        ~packed_array()
        {
            destroy_all();
        }

        // This is a big, fixed data structure for holding resources
        packed_array(const packed_array&)            = delete;
        packed_array& operator=(const packed_array&) = delete;
        packed_array(packed_array&&)                 = delete;
        packed_array& operator=(packed_array&&)      = delete;

        // Iterators
        iterator begin()                  noexcept { return m_data.data(); }
        const_iterator begin()      const noexcept { return m_data.data(); }
        const_iterator cbegin()     const noexcept { return m_data.data(); }
        iterator end()                    noexcept { return m_data.data() + m_size; }
        const_iterator end()        const noexcept { return m_data.data() + m_size; }
        const_iterator cend()       const noexcept { return m_data.data() + m_size; }

        // Size and capacity
        constexpr size_t size()     const noexcept { return m_size;   }
        constexpr size_t max_size() const noexcept { return N; }
        constexpr bool empty()      const noexcept { return m_size == 0; }

        constexpr index_type key()  const noexcept { return static_cast<index_type>(m_size); }

        // Access
        inline reference operator[](index_type pos)
        {
            return m_data[pos];
        }

        inline const_reference operator[](index_type pos) const
        {
            return m_data[pos];
        }

        inline reference at(index_type pos)
        {
            if (pos >= m_size)
                throw std::out_of_range("packed_array is full");
            return m_data[pos];
        }

        inline const_reference at(index_type pos) const
        {
            if (pos >= m_size)
                throw std::out_of_range("packed_array is full");
            return m_data[pos];
        }

        // Operations
        template<typename ... Args>
        inline reference emplace_back(Args&& ... args)
        {
            T& result = m_data.emplace_at(m_size, std::forward<Args>(args) ...);
            ++m_size; // Important to increment after in case we throw
            return result;
        }

        inline void pop_back()
        {
            m_data.destroy_at(m_size - 1);
            --m_size;
        }

        inline void clear()
        {
            destroy_all();
            m_size = 0;
        }

    private:
        inline void destroy_all()
        {
            for (size_t idx = 0; idx < m_size; ++idx)
                m_data.destroy(idx);
        }

        size_t                   m_size;
        nonstd::raw_buffer<T, N> m_data;
    };
}
