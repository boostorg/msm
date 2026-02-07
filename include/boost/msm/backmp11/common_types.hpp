// Copyright 2025 Christian Granzin
// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACKMP11_COMMON_TYPES_H
#define BOOST_MSM_BACKMP11_COMMON_TYPES_H

#include <any>
#include <cstdint>
#include <optional>

#include <boost/msm/back/common_types.hpp>

namespace boost::msm::back
{

// Bitwise operations for process_result.
// Defined in this header instead of back because type_traits are C++11.
// Defined in the back namespace because the operations have to be in the
// same namespace as HandledEnum.

constexpr HandledEnum operator|(HandledEnum lhs, HandledEnum rhs)
{
    return static_cast<HandledEnum>(
        static_cast<std::underlying_type_t<HandledEnum>>(lhs) |
        static_cast<std::underlying_type_t<HandledEnum>>(rhs)
    );
}

constexpr HandledEnum& operator|=(HandledEnum& lhs, HandledEnum rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr HandledEnum operator&(HandledEnum lhs, HandledEnum rhs)
{
    return static_cast<HandledEnum>(
        static_cast<std::underlying_type_t<HandledEnum>>(lhs) &
        static_cast<std::underlying_type_t<HandledEnum>>(rhs)
    );
}

constexpr HandledEnum& operator&=(HandledEnum& lhs, HandledEnum rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

} // namespace boost::msm::back

namespace boost::msm::backmp11
{

using process_result = back::HandledEnum;

using any_event = std::any;

// Selector for the visit mode.
// Can be active_states or all_states in recursive or non-recursive mode.
enum class visit_mode
{
    // State selection (mutually exclusive).
    active_states = 0b001,
    all_states    = 0b010,

    // Traversal mode (not set = non-recursive).
    recursive     = 0b100,

    // All valid combinations.
    active_non_recursive = active_states,
    active_recursive     = active_states | recursive,
    all_non_recursive    = all_states,
    all_recursive        = all_states | recursive
};
constexpr visit_mode operator|(visit_mode lhs, visit_mode rhs)
{
    return static_cast<visit_mode>(
        static_cast<std::underlying_type_t<visit_mode>>(lhs) |
        static_cast<std::underlying_type_t<visit_mode>>(rhs)
    );
}

namespace detail
{

// Occurrence of an event.
// Event occurrences are placed in an event pool for later processing.
class event_occurrence
{
  public:
    virtual ~event_occurrence() = default;

    // Try to process the event.
    // A return value std::nullopt means that the conditions for processing
    // were not given and the event has not been dispatched.
    virtual std::optional<process_result> process(uint16_t seq_cnt) = 0;

    // Flag set when this event has been processed and can be erased.
    // Deletion is deferred to allow the use of std::deque,
    // which provides better cache locality and lower per-element overhead.
    bool marked_for_deletion{};
};

// Additional info required for event processing.
enum class process_info
{
    direct_call,
    submachine_call,
    event_pool
};

// Bitmask for process result checks.
static constexpr process_result handled_true_or_deferred =
    process_result::HANDLED_TRUE | process_result::HANDLED_DEFERRED;

// Minimal implementation of a unique_ptr.
// Used to keep header dependencies to a minimum
// (from C++20 the memory header includes ostream).
template <typename T>
class basic_unique_ptr
{
  public:
    explicit basic_unique_ptr(T* ptr) noexcept : m_ptr(ptr)
    {
    }

    ~basic_unique_ptr()
    {
        delete m_ptr;
    }
    
    basic_unique_ptr(const basic_unique_ptr&) = delete;
    basic_unique_ptr& operator=(const basic_unique_ptr&) = delete;

    basic_unique_ptr(basic_unique_ptr&& other) noexcept : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    basic_unique_ptr& operator=(basic_unique_ptr&& other) noexcept
    {
        delete m_ptr;
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }

    T* get() const noexcept
    {
        return m_ptr;
    }

    T& operator*() const noexcept
    {
        return *m_ptr;
    }

    T* operator->() const noexcept
    {
        return m_ptr;
    }

  private:
    T* m_ptr{};
};

} // namespace detail
} // namespace boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_COMMON_TYPES_H
