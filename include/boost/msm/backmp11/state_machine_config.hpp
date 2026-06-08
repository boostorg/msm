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

#ifndef BOOST_MSM_BACKMP11_STATE_MACHINE_CONFIG_HPP
#define BOOST_MSM_BACKMP11_STATE_MACHINE_CONFIG_HPP

#include <deque>

namespace boost::msm::backmp11
{

namespace detail
{

struct config_tag {};
// Check whether a type is a config.
template <class T>
using is_config = std::is_same<typename T::internal::tag, config_tag>;

} // namespace detail

/// No derived is configured.
/// See @ref state_machine.
struct no_derived {};

/// Configuration parameters to select how events are dispatched.
namespace dispatch_strategy
{

/** Generates a flat fold of inline comparison branches.

    The code can be optimized to a jump table.
    - + Best executable size and runtime speed for most compilers
    - + No indirection — fully inlinable
    - \- O(n) comparisons in the worst case
 */
struct flat_fold {};

/** Generates an array of function pointers.

    - + O(1) dispatch
    - + Slightly better compile times
    - \- Indirect call through function pointer — not inlinable
    - \- Larger executable size (one pointer per state per event type)
 */
struct function_pointer_array {};

} // namespace dispatch_strategy

/** Optimizes for runtime speed (see @ref
    state_machine_config::compile_policy).

    Provides the best runtime speed and executable size at the cost of
    increased compile time. 
 */
struct favor_runtime_speed
{
    /// Dispatch strategy for processing events. Defaults to @ref dispatch_strategy::flat_fold.
    using dispatch_strategy = dispatch_strategy::flat_fold;
};

/** Optimizes for compile time (see @ref
    state_machine_config::compile_policy).

    Trades lower compile time for bigger executable size and lower runtime
    speed. Does not support the following features:
   - event hierarchies
   - Kleene events
 */
struct favor_compile_time {};

/// No context is configured (see @ref state_machine_config::context).
struct no_context {};

/// No root state machine is configured (see @ref state_machine_config::root_sm).
struct no_root_sm {};

/// Passes the immediate parent machine as `Fsm` argument
/// (see @ref state_machine_config::fsm_parameter).
struct local_transition_owner {};

/// Deactivates the event pool (see @ref state_machine_config::event_pool_container).
template <typename T>
struct no_event_pool_container {};

/// No observer is configured (see @ref state_machine_config::observer).
struct no_observer {};

/// Default state machine configuration.
struct default_state_machine_config
{
    /** Sets up a context accessible by all (sub-)machines in hierarchical state
        machines. Defaults to @ref no_context.

        Requires a reference to the context for state machine construction.
     */
    using context = no_context;
    /** Optimizes for runtime speed or compile time.
        Defaults to @ref favor_runtime_speed.

        The compile policy affects characteristics related to compile time,
        runtime speed, code size, and available features.
     */
    using compile_policy = favor_runtime_speed;
    /**
     * @brief Configures the container type of the event pool. Defaults to
     * `std::deque`.
     *
     * The event pool is required to handle deferred events, enqueued events,
     * and completion transitions.
     */
    template <typename T>
    using event_pool_container = std::deque<T>;
    /// Type of the Fsm parameter passed in actions and guards.
    /// Defaults to @ref local_transition_owner.
    using fsm_parameter = local_transition_owner;
    /// Sets up an observer for monitoring state machine activities.
    using observer = no_observer;
    /// Identifies the upper-most machine in hierarchical state machines.
    /// Defaults to @ref no_root_sm.
    using root_sm = no_root_sm;

    struct internal
    {
        using tag = detail::config_tag;
    };
};

/// Alias for @ref default_state_machine_config.
using state_machine_config = default_state_machine_config;

} // namespace boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_STATE_MACHINE_CONFIG_HPP
