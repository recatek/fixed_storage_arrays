#define CATCH_CONFIG_MAIN
#include "test.h"

#include "../include/keyed_array.h"
#include "../include/packed_array.h"
#include "../include/push_array.h"
#include "../include/raw_buffer.h"
#include "../include/slot_array.h"
#include "../include/versioned_key.h"

using namespace testing;

namespace test_raw_buffer
{
    template<typename TVal>
    struct test_destroy
    {
        using val_type = TVal;

        template<typename T>
        static void destroy(T& raw_buf, size_t idx)
        {
            raw_buf.destroy(idx);
        }
    };

    template<typename TVal>
    struct test_destroy_at
    {
        using val_type = TVal;

        template<typename T>
        static void destroy(T& raw_buf, size_t idx)
        {
            raw_buf.destroy_at(idx);
        }
    };

    TEMPLATE_PRODUCT_TEST_CASE(
        "nonstd::raw_buffer test cases",
        "[nonstd][raw-buffer]",
        (test_destroy, test_destroy_at),
        (val_t<0>, val_t<1>, val_t<20>, val_t<100>))
    {
        constexpr size_t size = TestType::val_type::val;

        auto arr = std::array<int64_t, size>();
        auto refcount = std::array<int32_t, size>();
        auto buf = nonstd::raw_buffer<ref_proxy, size>();

        for (size_t idx = 0; idx < size; ++idx)
        {
            arr[idx] = idx;
            buf.emplace_at(idx, arr[idx], &refcount[idx]);
        }

        REQUIRE(ref_proxy::test_refs(refcount, 1));

        SECTION("the elements sum correctly")
        {
            int64_t sum_expect = ((size - 1) * size) / 2;
            int64_t sum_arr = 0;
            int64_t sum_buf = 0;

            for (size_t idx = 0; idx < size; ++idx)
            {
                sum_arr += arr[idx];
                sum_buf += buf[idx].value();
            }

            REQUIRE(sum_expect == sum_arr);
            REQUIRE(sum_expect == sum_buf);
        }

        SECTION("an element removed is properly destroyed")
        {
            if (int64_t index = min_index(size, 2); index >= 0)
            {
                TestType::destroy(buf, index);
                REQUIRE(refcount[index] == 0);

                buf.emplace(index, 40, &refcount[index]);
                REQUIRE(refcount[index] == 1);
                REQUIRE(buf[index].value() == 40);
            }
        }

        SECTION("erroneous reconstruction of an element")
        {
            if (int64_t index = min_index(size, 5); index >= 0)
            {
                buf.emplace_at(index, 30, &refcount[index]);

                REQUIRE(refcount[index] == 2);
                REQUIRE(buf[index].value() == 30);
            }
        }
    }
}

namespace test_keyed_array
{
    template<typename TVal>
    struct test_clear
    {
        using val_type = TVal;

        template<typename T, typename TKeys>
        static bool clear(T& structure, TKeys&)
        {
            structure.clear();
            return true;
        }
    };

    template<typename TVal>
    struct test_remove
    {
        using val_type = TVal;

        template<typename T, typename TKeys>
        static bool clear(T& structure, TKeys& keys)
        {
            bool result = true;
            for (size_t idx = 0; idx < structure.max_size(); ++idx)
                result &= structure.try_remove(keys[idx]);
            return result;
        }
    };

    template<typename T>
    auto test_emplace(
        T& structure,
        int64_t value,
        int32_t* dummy,
        int16_t metadata = 0)
    {
        return structure.template emplace_back<int64_t, int32_t*>(
            std::move(value),
            std::move(dummy),
            metadata);
    }

    TEMPLATE_PRODUCT_TEST_CASE(
        "nonstd::keyed_array test cases",
        "[nonstd][keyed-array]",
        (test_clear, test_remove),
        (val_t<0>, val_t<1>, val_t<20>, val_t<100>))
    {
        constexpr size_t size = TestType::val_type::val;
        using structure_type = nonstd::keyed_array<ref_proxy, size>;
        using key_array = std::array<typename structure_type::key_type, size>;

        auto arr = test_range<int64_t, size>();
        auto refcount = std::array<int32_t, size>();

        /* Do not use a section tag here! */
        {
            auto structure = structure_type();
            auto keys = key_array();

            for (size_t idx = 0; idx < structure.max_size(); ++idx)
            {
                keys[idx] = test_emplace(structure, arr[idx], &refcount[idx]);
            }

            SECTION("the structure is filled properly")
            {
                REQUIRE(ref_proxy::test_refs(refcount, 1));
                REQUIRE(structure.full());

                for (size_t idx = 0; idx < size; ++idx)
                {
                    REQUIRE(keys[idx]);
                    REQUIRE(structure.try_get(keys[idx]) != nullptr);
                    REQUIRE(structure.try_get(keys[idx])->value() == arr[idx]);
                }
            }

            SECTION("the structure throws if added to")
            {
                int32_t dummy = 0;
                REQUIRE_THROWS_AS(
                    test_emplace(structure, 0, &dummy),
                    std::out_of_range);
            }

            SECTION("individual elements can be invalidated")
            {
                if (int64_t index = min_index(size, 2); index >= 0)
                {
                    bool result = structure.try_remove(keys[index]);

                    REQUIRE(result);
                    REQUIRE(structure.full() == false);
                    REQUIRE(structure.try_get(keys[index]) == nullptr);

                    SECTION("we can add new elements successfully")
                    {
                        int64_t value = size;
                        int32_t dummy = 0;
                        auto key = test_emplace(structure, value, &dummy, 24);

                        REQUIRE(key);
                        REQUIRE(key.meta() == 24);

                        REQUIRE(structure.try_get(key) != nullptr);
                        REQUIRE(structure.try_get(key)->value() == value);
                        REQUIRE(structure.try_get(keys[index]) == nullptr);

                        REQUIRE(structure.full());
                    }
                }
            }

            SECTION("clearing works correctly")
            {
                bool result = TestType::clear(structure, keys);

                REQUIRE(result);
                REQUIRE(structure.full() == (size == 0));
                REQUIRE(ref_proxy::test_refs(refcount, 0));

                for (size_t idx = 0; idx < size; ++idx)
                {
                    REQUIRE(structure.try_get(keys[idx]) == nullptr);
                }

                if constexpr (size > 0)
                {
                    SECTION("adding a new element is successful")
                    {
                        int32_t dummy = 0;
                        REQUIRE(test_emplace(structure, 0, &dummy));
                        REQUIRE(structure.full() == (size <= 1));
                    }
                }

                SECTION("repopulating after a clear works correctly")
                {
                    auto keys2 = key_array();
                    int64_t offset = static_cast<int64_t>(size);

                    for (size_t idx = 0; idx < structure.max_size(); ++idx)
                    {
                        auto val = arr[idx] + offset;
                        keys2[idx] =
                            test_emplace(structure, val, &refcount[idx]);

                        REQUIRE(structure.try_get(keys[idx]) == nullptr);

                        REQUIRE(keys2[idx]);
                        REQUIRE(structure.try_get(keys2[idx]) != nullptr);
                        REQUIRE(structure.try_get(keys2[idx])->value() == val);
                    }

                    REQUIRE(ref_proxy::test_refs(refcount, 1));
                    REQUIRE(structure.full());

                    SECTION("the structure throws if added to")
                    {
                        int32_t dummy = 0;
                        REQUIRE_THROWS_AS(
                            test_emplace(structure, 0, &dummy),
                            std::out_of_range);
                    }
                }
            }
        }

        REQUIRE(ref_proxy::test_refs(refcount, 0));
    }
}

namespace test_slot_array
{
    template<typename TVal>
    struct test_clear
    {
        using val_type = TVal;

        template<typename T, typename TKeys>
        static bool clear(T& structure, TKeys&)
        {
            structure.clear();
            return true;
        }
    };

    template<typename TVal>
    struct test_remove
    {
        using val_type = TVal;

        template<typename T, typename TKeys>
        static bool clear(T& structure, TKeys& keys)
        {
            bool result = true;
            for (size_t idx = 0; idx < structure.max_size(); ++idx)
                result &= structure.try_remove(keys[idx]);
            return result;
        }
    };

    template<typename T>
    auto test_emplace(
        T& structure,
        int64_t value,
        int32_t* dummy,
        int16_t metadata = 0)
    {
        return structure.template emplace_back<int64_t, int32_t*>(
            std::move(value),
            std::move(dummy),
            metadata);
    }

    TEMPLATE_PRODUCT_TEST_CASE(
        "nonstd::slot_array test cases",
        "[nonstd][slot-array]",
        (test_clear, test_remove),
        (val_t<0>, val_t<1>, val_t<20>, val_t<100>))
    {
        constexpr size_t size = TestType::val_type::val;
        using structure_type = nonstd::slot_array<ref_proxy, size>;
        using key_array = std::array<typename structure_type::key_type, size>;

        auto arr = test_range<int64_t, size>();
        auto refcount = std::array<int32_t, size>();

        /* Do not use a section tag here! */
        {
            auto structure = structure_type();
            auto keys = key_array();

            for (size_t idx = 0; idx < structure.max_size(); ++idx)
            {
                keys[idx] = test_emplace(structure, arr[idx], &refcount[idx]);
            }

            SECTION("the structure is filled properly")
            {
                REQUIRE(ref_proxy::test_refs(refcount, 1));
                REQUIRE(structure.size() == size);

                for (size_t idx = 0; idx < size; ++idx)
                {
                    REQUIRE(keys[idx]);
                    REQUIRE(structure.try_get(keys[idx]) != nullptr);
                    REQUIRE(structure.try_get(keys[idx])->value() == arr[idx]);
                }
            }

            SECTION("iterating the structure matches expectations")
            {
                int64_t sum_expected = 0;
                int64_t sum_computed = 0;

                for (auto& val : arr)
                    sum_expected += val;
                for (auto& val : structure)
                    sum_computed += val.value();

                REQUIRE(sum_expected == sum_computed);
            }

            SECTION("the structure throws if added to")
            {
                int32_t dummy = 0;
                REQUIRE_THROWS_AS(
                    test_emplace(structure, 0, &dummy),
                    std::out_of_range);
            }

            SECTION("individual elements can be invalidated")
            {
                if (int64_t index = min_index(size, 2); index >= 0)
                {
                    bool result = structure.try_remove(keys[index]);

                    REQUIRE(result);
                    REQUIRE(structure.size() == (size - 1));
                    REQUIRE(structure.try_get(keys[index]) == nullptr);

                    SECTION("we can add new elements successfully")
                    {
                        int64_t value = size;
                        int32_t dummy = 0;
                        auto key = test_emplace(structure, value, &dummy, 24);

                        REQUIRE(key);
                        REQUIRE(key.meta() == 24);

                        REQUIRE(structure.try_get(key) != nullptr);
                        REQUIRE(structure.try_get(key)->value() == value);
                        REQUIRE(structure.try_get(keys[index]) == nullptr);

                        REQUIRE(structure.size() == size);
                    }

                    SECTION("iterating the structure matches expectations")
                    {
                        int64_t sum_expected = 0 - arr[index];
                        int64_t sum_computed = 0;

                        for (auto& val : arr)
                            sum_expected += val;
                        for (auto& val : structure)
                            sum_computed += val.value();

                        REQUIRE(sum_expected == sum_computed);
                    }
                }
            }

            SECTION("clearing works correctly")
            {
                bool result = TestType::clear(structure, keys);

                REQUIRE(result);
                REQUIRE(structure.size() == 0);
                REQUIRE(ref_proxy::test_refs(refcount, 0));

                for (size_t idx = 0; idx < size; ++idx)
                {
                    REQUIRE(structure.try_get(keys[idx]) == nullptr);
                }


                if constexpr (size > 0)
                {
                    SECTION("adding a new element is successful")
                    {
                        int32_t dummy = 0;
                        REQUIRE(test_emplace(structure, 0, &dummy));
                        REQUIRE(structure.size() == 1);
                    }
                }

                SECTION("repopulating after a clear works correctly")
                {
                    auto keys2 = key_array();
                    int64_t offset = static_cast<int64_t>(size);

                    for (size_t idx = 0; idx < structure.max_size(); ++idx)
                    {
                        auto val = arr[idx] + offset;
                        keys2[idx] =
                            test_emplace(structure, val, &refcount[idx]);

                        REQUIRE(structure.try_get(keys[idx]) == nullptr);

                        REQUIRE(keys2[idx]);
                        REQUIRE(structure.try_get(keys2[idx]) != nullptr);
                        REQUIRE(structure.try_get(keys2[idx])->value() == val);
                    }

                    REQUIRE(ref_proxy::test_refs(refcount, 1));
                    REQUIRE(structure.size() == size);

                    SECTION("iterating the structure matches expectations")
                    {
                        int64_t sum_expected = 0;
                        int64_t sum_computed = 0;

                        for (auto& val : arr)
                            sum_expected += val + offset;
                        for (auto& val : structure)
                            sum_computed += val.value();

                        REQUIRE(sum_expected == sum_computed);
                    }

                    SECTION("the structure throws if added to")
                    {
                        int32_t dummy = 0;
                        REQUIRE_THROWS_AS(
                            test_emplace(structure, 0, &dummy),
                            std::out_of_range);
                    }
                }
            }
        }

        REQUIRE(ref_proxy::test_refs(refcount, 0));
    }
}

namespace test_packed_array
{
    template<typename TVal>
    struct test_clear
    {
        using val_type = TVal;

        template<typename T>
        static void clear(T& structure)
        {
            structure.clear();
        }
    };

    template<typename T>
    void test_emplace(
        T& structure,
        int64_t value,
        int32_t* dummy)
    {
        structure.template emplace_back<int64_t, int32_t*>(
            std::move(value),
            std::move(dummy));
    }

    template<typename T>
    int64_t test_sum(T& structure)
    {
        int64_t sum = 0;
        for (auto& proxy : structure)
            sum += proxy.value();
        return sum;
    }

    TEMPLATE_PRODUCT_TEST_CASE(
        "nonstd::packed_array test cases",
        "[nonstd][packed-array]",
        (test_clear),
        (val_t<0>, val_t<1>, val_t<20>, val_t<100>))
    {
        constexpr size_t size = TestType::val_type::val;
        using structure_type = nonstd::packed_array<ref_proxy, size>;

        auto arr = test_range<int64_t, size>();
        auto refcount = std::array<int32_t, size>();

        /* Do not use a section tag here! */
        {
            auto structure = structure_type();

            for (size_t idx = 0; idx < structure.max_size(); ++idx)
            {
                test_emplace(structure, arr[idx], &refcount[idx]);
            }

            SECTION("the structure is filled properly")
            {
                REQUIRE(ref_proxy::test_refs(refcount, 1));
                REQUIRE(structure.size() == size);
            }

            SECTION("iterating over the structure matches expectations")
            {
                int64_t sum_expected = 0;
                int64_t sum_computed = 0;

                for (auto& val : arr)
                    sum_expected += val;
                for (auto& val : structure)
                    sum_computed += val.value();

                REQUIRE(sum_expected == sum_computed);
            }

            SECTION("the structure throws if added to")
            {
                int32_t dummy = 0;
                REQUIRE_THROWS_AS(
                    test_emplace(structure, 0, &dummy),
                    std::out_of_range);
            }

            SECTION("clearing works correctly")
            {
                TestType::clear(structure);

                REQUIRE(structure.size() == 0);
                REQUIRE(ref_proxy::test_refs(refcount, 0));

                if constexpr (size > 0)
                {
                    SECTION("adding a new element is successful")
                    {
                        int32_t dummy = 0;
                        test_emplace(structure, 0, &dummy);
                        REQUIRE(structure.size() == 1);
                    }
                }

                SECTION("repopulating after a clear works correctly")
                {
                    int64_t offset = static_cast<int64_t>(size);

                    for (size_t idx = 0; idx < structure.max_size(); ++idx)
                    {
                        auto val = arr[idx] + offset;
                        test_emplace(structure, val, &refcount[idx]);
                    }

                    REQUIRE(ref_proxy::test_refs(refcount, 1));
                    REQUIRE(structure.size() == size);

                    SECTION("iterating the structure matches expectations")
                    {
                        int64_t sum_expected = 0;
                        int64_t sum_computed = 0;

                        for (auto& val : arr)
                            sum_expected += val + offset;
                        for (auto& val : structure)
                            sum_computed += val.value();

                        REQUIRE(sum_expected == sum_computed);
                    }

                    SECTION("the structure throws if added to")
                    {
                        int32_t dummy = 0;
                        REQUIRE_THROWS_AS(
                            test_emplace(structure, 0, &dummy),
                            std::out_of_range);
                    }
                }
            }
        }

        REQUIRE(ref_proxy::test_refs(refcount, 0));
    }
}
