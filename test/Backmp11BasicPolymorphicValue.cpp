// Copyright 2026 Christian Granzin
// Copyright 2024 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE backmp11_basic_polymorphic_value
#endif
#include <boost/test/unit_test.hpp>

#include <boost/msm/backmp11/detail/basic_polymorphic_value.hpp>
#include "Utils.hpp"

using boost::msm::backmp11::detail::basic_polymorphic_value;

namespace
{

struct trivial_class
{
    int data;
};

BOOST_AUTO_TEST_CASE(trivial_class_test)
{
    using ptr_t = basic_polymorphic_value<trivial_class>;
    static_assert(sizeof(ptr_t) == 64);

    ptr_t ptr = ptr_t::make(trivial_class{42});
    BOOST_REQUIRE(ptr->data == 42);
}

struct class_with_destructor
{
    class_with_destructor(int value) : value(value) {}

    ~class_with_destructor()
    {
        destructor_calls++;
    }

    static size_t destructor_calls;

    int value;
};
size_t class_with_destructor::destructor_calls{};
BOOST_AUTO_TEST_CASE(class_with_destructor_test)
{
    using ptr_t = basic_polymorphic_value<class_with_destructor>;

    [[maybe_unused]] size_t& destructor_calls = class_with_destructor::destructor_calls;

   {
        ptr_t ptr = ptr_t::make<class_with_destructor>(42);
        BOOST_REQUIRE(ptr->value == 42);
        BOOST_REQUIRE(ptr.is_inline());

        ptr_t ptr_2 = std::move(ptr);
        BOOST_REQUIRE(ptr_2->value == 42);
    }
    // 2..3 destructor calls:
    // 1. ptr's object destroyed at end of scope
    // 2. ptr_2's object destroyed at end of scope
    // 3. MSVC doesn't apply copy elision when built in debug mode.
    BOOST_REQUIRE(class_with_destructor::destructor_calls == 2 ||
                  class_with_destructor::destructor_calls == 3);
    class_with_destructor::destructor_calls = 0;

    {
        ptr_t ptr = ptr_t::make(class_with_destructor{42});
        BOOST_REQUIRE(ptr->value == 42);
    }
    // 2..3 destructor calls:
    // 1. Temporary object destroyed after being moved into ptr
    // 2. ptr's object destroyed at end of scope
    // 3. GCC<14 cannot apply copy elision
    BOOST_REQUIRE(class_with_destructor::destructor_calls == 2 ||
                  class_with_destructor::destructor_calls == 3);
    class_with_destructor::destructor_calls = 0;
}

struct big_class_with_destructor
{
    big_class_with_destructor(uint8_t value) : value(value)
    {
    }

    ~big_class_with_destructor()
    {
        destructor_calls++;
    }

    static size_t destructor_calls;

    uint8_t value;
    std::array<uint8_t, 128> data;
};
size_t big_class_with_destructor::destructor_calls{};
BOOST_AUTO_TEST_CASE(big_class_with_destructor_test)
{
    using ptr_t = basic_polymorphic_value<big_class_with_destructor>;
    {
        ptr_t ptr = ptr_t::make<big_class_with_destructor>(42);
        BOOST_REQUIRE(!ptr.is_inline());
        BOOST_REQUIRE(ptr->value == 42);

        ptr_t ptr_2 = std::move(ptr);
        BOOST_REQUIRE(ptr_2->value == 42);
    }
    // Only one destructor call, because the value is on the heap.
    ASSERT_AND_RESET(big_class_with_destructor::destructor_calls, 1);
}

struct sub_class__with_destructor : class_with_destructor
{
    sub_class__with_destructor(int data) : class_with_destructor(data) {}

    ~sub_class__with_destructor()
    {
        destructor_calls++;
    }

    static size_t destructor_calls;

    int data;
};
size_t sub_class__with_destructor::destructor_calls{};
BOOST_AUTO_TEST_CASE(two_destructors_test)
{
    using ptr_t = basic_polymorphic_value<class_with_destructor>;

    [[maybe_unused]] size_t& destructor_calls_0 = class_with_destructor::destructor_calls;
    [[maybe_unused]] size_t& destructor_calls_1 = sub_class__with_destructor::destructor_calls;

    {
        ptr_t ptr = ptr_t::make<sub_class__with_destructor>(42);
        BOOST_REQUIRE(ptr->value == 42);
        BOOST_REQUIRE(ptr.is_inline());
    }
    // 1..2 destructor calls:
    // 1. ptr's object destroyed at end of scope
    // 2. MSVC doesn't apply copy elision when built in debug mode.
    BOOST_REQUIRE(class_with_destructor::destructor_calls == 1 ||
                  class_with_destructor::destructor_calls == 2);
    class_with_destructor::destructor_calls = 0;
    BOOST_REQUIRE(sub_class__with_destructor::destructor_calls == 1 ||
                  sub_class__with_destructor::destructor_calls == 2);
    sub_class__with_destructor::destructor_calls = 0;

    {
        ptr_t ptr = ptr_t::make(sub_class__with_destructor{42});
        BOOST_REQUIRE(ptr->value == 42);
    }
    // 2..3 destructor calls:
    // 1. Temporary object destroyed after being moved into ptr
    // 2. ptr's object destroyed at end of scope
    // 3. GCC<14 cannot apply copy elision.
    BOOST_REQUIRE(class_with_destructor::destructor_calls == 2 ||
                  class_with_destructor::destructor_calls == 3);
    class_with_destructor::destructor_calls = 0;
    BOOST_REQUIRE(sub_class__with_destructor::destructor_calls == 2 ||
                  sub_class__with_destructor::destructor_calls == 3);
    sub_class__with_destructor::destructor_calls = 0;
}

struct virtual_class
{
    virtual void method()
    {
        method_calls++;
    }

    static size_t method_calls;
};
size_t virtual_class::method_calls{};
struct other_virtual_class : virtual_class
{
    void method() override
    {
        method_calls++;
    }

    static size_t method_calls;
};
size_t other_virtual_class::method_calls{};
BOOST_AUTO_TEST_CASE(virtual_class_test)
{
    using ptr_t = basic_polymorphic_value<virtual_class>;

    ptr_t ptr = ptr_t::make<virtual_class>();
    BOOST_REQUIRE(ptr.is_inline());
    ptr->method();
    ASSERT_AND_RESET(virtual_class::method_calls, 1);
    
    ptr_t other_ptr = ptr_t::make<other_virtual_class>();
    ptr = std::move(other_ptr);
    ptr->method();
    ASSERT_AND_RESET(other_virtual_class::method_calls, 1);
}

} // namespace
