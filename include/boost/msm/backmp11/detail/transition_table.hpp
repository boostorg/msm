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

#ifndef BOOST_MSM_BACKMP11_DETAIL_TRANSITION_TABLE_HPP
#define BOOST_MSM_BACKMP11_DETAIL_TRANSITION_TABLE_HPP

#include "boost/assert.hpp"

#include <boost/msm/active_state_switching_policies.hpp>
#include <boost/msm/backmp11/detail/metafunctions.hpp>

namespace boost::msm::backmp11::detail
{

template <typename StateMachine>
struct transition_table_impl
{
    using front_end_t = typename StateMachine::front_end_t;
    using derived_t = typename StateMachine::derived_t;
    using state_set = typename StateMachine::state_set;

    template<typename T>
    using get_active_state_switch_policy = typename T::active_state_switch_policy;
    using active_state_switching =
        boost::mp11::mp_eval_or<active_state_switch_after_entry,
                                get_active_state_switch_policy, front_end_t>;

    template<typename Row, bool HasGuard, typename Event, typename Source, typename Target>
    static bool call_guard_or_true(StateMachine& sm, const Event& event, Source& source, Target& target)
    {
        if constexpr (HasGuard)
        {
            return Row::guard_call(sm.get_fsm_argument(), event, source, target, sm.m_states);
        }
        else
        {
            return true;
        }
    }
    template<typename Row, bool HasAction, typename Event, typename Source, typename Target>
    static process_result call_action_or_true(StateMachine& sm, const Event& event, Source& source, Target& target)
    {
        if constexpr (HasAction)
        {
            return Row::action_call(sm.get_fsm_argument(), event, source, target, sm.m_states);
        }
        else
        {
            return process_result::HANDLED_TRUE;
        }
    }

    // Template used to create transitions from rows in the transition table.
    template<typename Row, bool HasAction, bool HasGuard>
    struct transition
    {
        using transition_event = typename Row::Evt;
        using current_state_type = convert_source_state<derived_t, typename Row::Source>;
        using next_state_type = convert_target_state<derived_t, typename Row::Target>;

        // Take the transition action and return the next state.
        static process_result execute(StateMachine& sm,
                                      int& state_id,
                                      transition_event const& event)
        {
            static constexpr int current_state_id = StateMachine::template get_state_id<current_state_type>();
            static constexpr int next_state_id = StateMachine::template get_state_id<next_state_type>();
            BOOST_ASSERT(state_id == current_state_id);

            auto& source = sm.template get_state<current_state_type>();
            auto& target = sm.template get_state<next_state_type>();

            if (!call_guard_or_true<Row, HasGuard>(sm, event, source, target))
            {
                // guard rejected the event, we stay in the current one
                return process_result::HANDLED_GUARD_REJECT;
            }
            state_id = active_state_switching::after_guard(current_state_id,next_state_id);

            // first call the exit method of the current state
            source.on_exit(event, sm.get_fsm_argument());
            state_id = active_state_switching::after_exit(current_state_id,next_state_id);

            // then call the action method
            process_result res = call_action_or_true<Row, HasAction>(sm, event, source, target);
            state_id = active_state_switching::after_action(current_state_id,next_state_id);

            // and finally the entry method of the new current state
            StateMachine::template convert_event_and_execute_entry<typename Row::Target>(target,event,sm);
            state_id = active_state_switching::after_entry(current_state_id,next_state_id);

            return res;
        }
    };

    // Template used to create internal transitions from rows in the transition table.
    template<typename Row, bool HasAction, bool HasGuard, typename State = typename Row::Source>
    struct internal_transition
    {
        using transition_event = typename Row::Evt;
        using current_state_type = State;
        using next_state_type = current_state_type;

        // Take the transition action and return the next state.
        static process_result execute(StateMachine& sm,
                                      [[maybe_unused]] int& state_id,
                                      transition_event const& event)
        {
            BOOST_ASSERT(state_id == StateMachine::template get_state_id<current_state_type>());
            
            auto& source = sm.template get_state<current_state_type>();
            auto& target = source;

            if (!call_guard_or_true<Row, HasGuard>(sm, event, source, target))
            {
                return process_result::HANDLED_GUARD_REJECT;
            }
            return call_action_or_true<Row, HasAction>(sm, event, source, target);
        }
    };

    // Template used to create sm-internal transitions from rows in the transition table.
    template<typename Row, bool HasAction, bool HasGuard>
    struct internal_transition<Row, HasAction, HasGuard, StateMachine>
    {
        using transition_event = typename Row::Evt;

        // Take the transition action and return the next state.
        static process_result execute(StateMachine& sm,
                                      transition_event const& event)
        {
            auto& source = sm;
            auto& target = source;

            if (!call_guard_or_true<Row, HasGuard>(sm, event, source, target))
            {
                return process_result::HANDLED_GUARD_REJECT;
            }
            return call_action_or_true<Row, HasAction>(sm, event, source, target);
        }
    };

    // Helpers used to create back-end transitions
    // from front-end transitions.
    template <class Tag, class Row, class State>
    struct create_transition_impl;
    template <class Row, class State>
    struct create_transition_impl<g_row_tag, Row, State>
    {
        using type = transition<Row, false, true>;
    };
    template <class Row, class State>
    struct create_transition_impl<a_row_tag, Row, State>
    {
        using type = transition<Row, true, false>;
    };
    template <class Row, class State>
    struct create_transition_impl<_row_tag, Row, State>
    {
        using type = transition<Row, false, false>;
    };
    template <class Row, class State>
    struct create_transition_impl<row_tag, Row, State>
    {
        using type = transition<Row, true, true>;
    };
    template <class Row, class State>
    struct create_transition_impl<g_irow_tag, Row, State>
    {
        using type = internal_transition<Row, false, true>;
    };
    template <class Row, class State>
    struct create_transition_impl<a_irow_tag, Row, State>
    {
        using type = internal_transition<Row, true, false>;
    };
    template <class Row, class State>
    struct create_transition_impl<irow_tag, Row, State>
    {
        using type = internal_transition<Row, true, true>;
    };
    template <class Row, class State>
    struct create_transition_impl<_irow_tag, Row, State>
    {
        using type = internal_transition<Row, false, false>;
    };
    template <class Row, class State>
    struct create_transition_impl<sm_a_i_row_tag, Row, State>
    {
        using type = internal_transition<Row, true, false, State>;
    };
    template <class Row, class State>
    struct create_transition_impl<sm_g_i_row_tag, Row, State>
    {
        using type = internal_transition<Row, false, true, State>;
    };
    template <class Row, class State>
    struct create_transition_impl<sm_i_row_tag, Row, State>
    {
        using type = internal_transition<Row, true, true, State>;
    };
    template <class Row, class State>
    struct create_transition_impl<sm__i_row_tag, Row, State>
    {
        using type = internal_transition<Row, false, false, State>;
    };
    template <class Row, class State>
    using create_transition =
        typename create_transition_impl<typename Row::row_type_tag, Row,
                                        State>::type;

    // Transform the front-end's internal transition table
    // to a back-end transition table,
    // with reversed ordering for backward compatibility.
    using internal_transition_table = mp11::mp_reverse<
        mp11::mp_transform_q<
            mp11::mp_bind_back<create_transition, StateMachine>,
            to_mp_list_t<typename front_end_t::internal_transition_table>>>;

    // Expand the front-end's transition table(s)
    // to a single back-end transition table,
    // with reversed ordering for backward compatibility.
    // First transform the main transition table of the SM.
    using sm_transition_table = mp11::mp_transform_q<
        mp11::mp_bind_back<create_transition, StateMachine>,
        to_mp_list_t<typename front_end_t::transition_table>>;
    // Then transform and append the states' internal transition tables.
    template <typename State>
    using append_state_transition_tables_predicate = mp11::mp_or<
        is_composite<State>,
        mp11::mp_empty<to_mp_list_t<typename State::internal_transition_table>>>;
    template <typename TransitionTable, typename State>
    using append_internal_transition_table = mp11::mp_append<
        TransitionTable,
        mp11::mp_transform_q<
            mp11::mp_bind_back<create_transition, State>,
            to_mp_list_t<typename State::internal_transition_table>>>;
    using transition_table = mp11::mp_reverse<
        mp11::mp_fold<
            mp11::mp_remove_if<state_set, append_state_transition_tables_predicate>,
            sm_transition_table,
            append_internal_transition_table>>;
};
template <typename StateMachine>
using transition_table = typename transition_table_impl<StateMachine>::transition_table;
template <typename StateMachine>
using internal_transition_table = typename transition_table_impl<StateMachine>::internal_transition_table;

}

#endif // BOOST_MSM_BACKMP11_DETAIL_TRANSITION_TABLE_HPP
