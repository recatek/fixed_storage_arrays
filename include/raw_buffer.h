#pragma once

#include <array>
#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>

namespace nonstd
{
    /// <summary>
    /// A raw buffer of typed but potentially uninitialized data.
    /// Values can be created and destroyed within this structure's slots.
    /// This structure does not track which slots are occupied and will not
    /// initialize its elements nor destroy them when destroyed itself.
    /// 
    /// Note that assigning to an uninitialized slot is undefined.
    /// Use emplacement instead to construct a value in that slot.
    /// </summary>
    template<typename T, std::size_t N>
    class raw_buffer
    {
    public:
        static constexpr auto capacity = N;

        raw_buffer()  = default;
        ~raw_buffer() = default;

        // Move and copy must be manually implemented per-element by the user
        // because we don't keep track of what is and isn't a valid slot
        raw_buffer(raw_buffer&& rhs)                 = delete;
        raw_buffer& operator=(raw_buffer&& rhs)      = delete;
        raw_buffer(const raw_buffer& rhs)            = delete;
        raw_buffer& operator=(const raw_buffer& rhs) = delete;

        // Size and capacity
        constexpr std::size_t max_size() const noexcept { return N; }

        // Access
        T* data()
        {
            return std::launder(
                reinterpret_cast<T*>(m_data.data()));
        }

        const T* data() const
        {
            return std::launder(
                reinterpret_cast<T const*>(m_data.data()));
        }

        inline T& operator[](std::size_t pos)
        {
            return *std::launder(
                reinterpret_cast<T*>(
                    std::addressof(m_data[pos])));
        }

        inline const T& operator[](std::size_t pos) const
        {
            return *std::launder(
                reinterpret_cast<const T*>(
                    std::addressof(m_data[pos])));
        }

        inline T& at(std::size_t pos)
        {
            return *std::launder(
                reinterpret_cast<T*>(
                    std::addressof(m_data.at(pos))));
        }

        inline const T& at(std::size_t pos) const
        {
            return *std::launder(
                reinterpret_cast<const T*>(
                    std::addressof(m_data.at(pos))));
        }

        // Operations
        template<typename ... Args>
        inline T& emplace(size_t pos, Args&&... args)
        {
            return
                *::new(static_cast<void*>(std::addressof(m_data[pos])))
                T(std::forward<Args>(args) ...);
        }

        template<typename ... Args>
        inline T& emplace_at(size_t pos, Args&&... args)
        {
            return
                *::new(static_cast<void*>(std::addressof(m_data.at(pos))))
                T(std::forward<Args>(args) ...);
        }

        inline void destroy(std::size_t pos)
        {
            std::destroy_at(
                std::launder(
                    reinterpret_cast<const T*>(
                        std::addressof(m_data[pos]))));
        }

        inline void destroy_at(std::size_t pos)
        {
            std::destroy_at(
                std::launder(
                    reinterpret_cast<const T*>(
                        std::addressof(m_data.at(pos)))));
        }

    private:
        std::array<std::aligned_storage_t<sizeof(T), alignof(T)>, N> m_data;
        static_assert( // TODO: The (N == 0) case has a size -- should it?
            (N == 0) || (sizeof(m_data) == (sizeof(T) * N)),
            "raw_buffer exhibiting incorrect sizing or alignment");
    };
}
