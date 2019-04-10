#pragma once

#include <array>
#include <stdexcept>

namespace nonstd
{
    /// <summary>
    /// An array intended for small, copyable data structures.
    /// Is itself trivially copyable and movable, but at the cost of
    /// requiring default-initialization and with no forwarding storage.
    /// Objects cannot be individually removed once added, only a full clear.
    /// </summary>
    template<class T, size_t N, typename Key = size_t>
    class push_array
    {
    public:
        using value_type        = T;
        using pointer           = T*;
        using const_pointer     = const T*;
        using reference         = value_type&;
        using const_reference   = const value_type&;
        using iterator          = value_type*;
        using const_iterator    = const value_type*;
        using index_type        = Key;

        static constexpr auto capacity = N;

    private:
        static const index_type max_index = std::numeric_limits<index_type>::max();
        static const index_type invalid_index = max_index;
        static_assert(N <= invalid_index, "packed_array too large for key");

    public:
        push_array()            = default;
        ~push_array()           = default;

        push_array(const push_array& rhs)            = default;
        push_array& operator=(const push_array& rhs) = default;
        push_array(push_array&& rhs)                 = default;
        push_array& operator=(push_array&& rhs)      = default;

        // Iterators
        iterator begin()                      noexcept { return m_data.data(); }
        const_iterator begin()          const noexcept { return m_data.data(); }
        const_iterator cbegin()         const noexcept { return m_data.data(); }
        iterator end()                        noexcept { return m_data.data() + m_size; }
        const_iterator end()            const noexcept { return m_data.data() + m_size; }
        const_iterator cend()           const noexcept { return m_data.data() + m_size; }

        // Size and N
        constexpr index_type size()     const noexcept { return m_size; }
        constexpr index_type max_size() const noexcept { return static_cast<index_type>(N); }
        constexpr bool empty()          const noexcept { return m_size == 0; }

        constexpr index_type key()  const noexcept { return static_cast<index_type>(m_size); }

        // Access
        reference operator[](index_type pos)
        {
            return m_data[pos];
        }

        const_reference operator[](index_type pos) const
        {
            return m_data[pos];
        }

        reference at(index_type pos)
        {
            if (pos >= m_size)
                throw std::out_of_range("push_array index out of range");
            return m_data[pos];
        }

        const_reference at(index_type pos) const
        {
            if (pos >= m_size)
                throw std::out_of_range("push_array index out of range");
            return m_data[pos];
        }

        // Operations
        template<typename ...Args>
        reference emplace_back(Args&&... args)
        {
            T& result = m_data.at(m_size);
            result = T(std::forward<Args>(args)...);
            ++m_size;
            return result;
        }

        index_type push_back(const_reference item)
        {
            const index_type index = static_cast<index_type>(m_size);
            m_data.at(index) = item;
            ++m_size;
            return index;
        }

    private:
        index_type       m_size;
        std::array<T, N> m_data;
    };
}
