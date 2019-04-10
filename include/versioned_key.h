#pragma once

#include <cstdint>

namespace nonstd
{
    struct versioned_key
    {
        template<class, size_t, typename> friend class slot_array;
        template<class, size_t, typename> friend class keyed_array;

    public:
        using version_type = uint32_t;
        using index_type   = uint16_t;
        using meta_type    = uint16_t;

        versioned_key() = default;

        bool is_null()    const noexcept { return (m_version == 0); }
        operator bool()   const noexcept { return (m_version != 0); }
        meta_type meta()  const noexcept { return m_meta; }

    private:
        versioned_key(
            version_type version,
            index_type index,
            meta_type meta)
            : m_version(version)
            , m_index(index)
            , m_meta(meta)
        {
            // Pass
        }

        version_type m_version;
        index_type   m_index;

    protected:
        meta_type    m_meta; // User metadata field, hidden unless overridden
    };
}
