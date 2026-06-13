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

#ifndef BOOST_MSM_BACKMP11_COMMON_TYPES_HPP
#define BOOST_MSM_BACKMP11_COMMON_TYPES_HPP

#include <cstdint>
#include <type_traits>

#include <boost/msm/back/common_types.hpp>

namespace boost::msm::backmp11
{

/// Describes the state of the state machine.
enum class machine_state : uint8_t
{
    /// Stopped / not started.
    stopped = 0,
    /// Ready to process an event.
    idle,
    /// Processing an event.
    processing
};

/// Return type of @ref state_machine::process_event calls.
using process_result = back::HandledEnum;

/// Default event when starting a state machine (see @ref state_machine::start).
struct starting {};

/// Default event when stopping a state machine (see @ref state_machine::stop).
struct stopping {};

/// Fold flags with a logical OR operation (see @ref state_machine::is_flag_active).
struct flag_or {};

/// Fold flags with a logical AND operation (see @ref state_machine::is_flag_active).
struct flag_and {};

/**
 * @brief Selector for the visit mode (see @ref state_machine::visit).
 *
 * Can be active_states or all_states in recursive or non-recursive mode.
 */
enum class visit_mode
{
    /// Visit only active states (mutually exclusive with all_states).
    active_states = 0b001,
    /// Visit all states (mutually exclusive with active_states).
    all_states    = 0b010,

    /// Visit states recursively (not set == not recursive).
    recursive     = 0b100,

    ///
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

} // namespace boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_COMMON_TYPES_HPP
