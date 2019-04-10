#include "catch.h"

namespace testing
{
    class ref_proxy
    {
        /// <summary>
        /// Custom swap to avoid invalid values on refcount.
        /// </summary>
        friend void swap(ref_proxy& lhs, ref_proxy& rhs)
        {
            using std::swap;

            swap(lhs.m_value, rhs.m_value);
            swap(lhs.m_refcount, rhs.m_refcount);
        }

    public:
        template<typename TArr>
        static bool test_refs(
            TArr& arr, 
            int32_t expected, 
            size_t start, 
            size_t end)
        {
            for (size_t idx = start; idx < end; ++idx)
                if (arr[idx] != expected)
                    return false;
            return true;
        }

        template<typename TArr>
        static bool test_refs(TArr& arr, int32_t expected)
        {
            return test_refs(arr, expected, 0, arr.max_size());
        }

        ref_proxy() = delete;

        ref_proxy(int64_t value, int32_t* refcount)
            : m_value(value)
            , m_refcount(refcount)
        {
            (*m_refcount)++;
        }

        ref_proxy(const ref_proxy& other)
            : m_value(other.m_value)
            , m_refcount(other.m_refcount) 
        {
            (*m_refcount)++;
        }

        ref_proxy(ref_proxy&& other)
            : m_value(other.m_value)
            , m_refcount(other.m_refcount)
        {
            other.m_refcount = nullptr;
        }

        ref_proxy& operator=(const ref_proxy& rhs)
        {
            ref_proxy tmp = rhs;
            swap(*this, tmp);
            return *this;
        }

        ref_proxy& operator=(ref_proxy&& rhs)
        {
            if (this != std::addressof(rhs))
            {
                if (m_refcount != nullptr)
                    (*m_refcount)--;
                m_value = rhs.m_value;
                m_refcount = rhs.m_refcount;
                rhs.m_refcount = nullptr;
            }

            return *this;
        }

        ~ref_proxy()
        {
            if (m_refcount != nullptr)
                (*m_refcount)--;
        }

        int64_t value() const
        {
            return m_value;
        }

    private:
        int64_t m_value;
        int32_t* m_refcount;
    };

    template<size_t Val>
    struct val_t
    {
        static const size_t val = Val;
    };

    template<size_t Val, typename TInner>
    struct prod_t
    {
        static const size_t val = Val;
        using inner_type = TInner;
    };

    template<typename TInner> using prod_t_0   = prod_t<0,   TInner>;
    template<typename TInner> using prod_t_1   = prod_t<1,   TInner>;
    template<typename TInner> using prod_t_20  = prod_t<20,  TInner>;
    template<typename TInner> using prod_t_100 = prod_t<100, TInner>;

    template<typename T, size_t Size>
    inline std::array<T, Size> test_range(T offset = 0)
    {
        auto arr = std::array<T, Size>();
        for (size_t idx = 0; idx < Size; ++idx)
            arr[idx] = static_cast<T>(idx) + offset;
        return arr;
    }

    inline int64_t min_index(size_t size, size_t index)
    {
        return std::min<int64_t>(static_cast<int64_t>(size) - 1, index);
    }
}
