// Copyright 2026 Christian Granzin
// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACKMP11_DETAIL_COMMON_HPP
#define BOOST_MSM_BACKMP11_DETAIL_COMMON_HPP

#include <cstdint>

#include <boost/msm/backmp11/common_types.hpp>

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
        static_cast<std::underlying_type_t<HandledEnum>>(rhs));
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
        static_cast<std::underlying_type_t<HandledEnum>>(rhs));
}

constexpr HandledEnum& operator&=(HandledEnum& lhs, HandledEnum rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

} // namespace boost::msm::back

namespace boost::msm::backmp11::detail
{

class process_guard
{
  public:
    explicit process_guard(machine_state& machine_state)
        : m_machine_state(machine_state)
    {
        m_machine_state = machine_state::processing;
    }

    ~process_guard()
    {
        m_machine_state = machine_state::idle;
    }

  private:
    machine_state& m_machine_state;
};

// Additional info required for event processing.
enum class process_info
{
    direct_call,
    submachine_call,
    event_pool
};

using process_result = back::HandledEnum;

// Bitmask for process result checks.
static constexpr process_result handled_true_or_deferred =
    process_result::HANDLED_TRUE | process_result::HANDLED_DEFERRED;

template <typename Policy, typename = void>
struct compile_policy_impl;

} // namespace boost::msm::backmp11::detail

#endif // BOOST_MSM_BACKMP11_DETAIL_COMMON_HPP
