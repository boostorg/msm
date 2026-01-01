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

#ifndef BOOST_MSM_BACKMP11_TRANSITION_TABLE_HPP
#define BOOST_MSM_BACKMP11_TRANSITION_TABLE_HPP

#include <boost/msm/backmp11/detail/metafunctions.hpp>

namespace boost::msm::backmp11::detail
{

// returns the transition table of a Composite state
template <class Derived>
struct get_transition_table
{
    using Stt = typename Derived::internal::template create_real_stt<typename Derived::front_end_t>::type;
    using type = Stt;
};
template<typename T>
using get_transition_table_t = typename get_transition_table<T>::type;

// recursively builds an internal table including those of substates, sub-substates etc.
// variant for submachines
template <class State, bool IsComposite>
struct recursive_get_internal_transition_table
{
    // get the composite's internal table
    typedef typename State::front_end_t::internal_transition_table composite_table;
    // and for every substate (state of submachine), recursively get the internal transition table
    using composite_states = typename State::internal::state_set;
    template<typename V, typename SubState>
    using append_recursive_internal_transition_table = mp11::mp_append<
        V,
        typename recursive_get_internal_transition_table<SubState, has_state_machine_tag<SubState>::value>::type
        >;
    typedef typename mp11::mp_fold<
        composite_states,
        to_mp_list_t<composite_table>,
        append_recursive_internal_transition_table
        > type;
};
// stop iterating on leafs (simple states)
template <class State>
struct recursive_get_internal_transition_table<State, false>
{
    typedef to_mp_list_t<
        typename State::internal_transition_table
        > type;
};
// recursively get a transition table for a given state machine.
// returns the transition table for this state + the tables of all submachines recursively.
template <class Composite>
struct recursive_get_transition_table
{
    // get the transition table of the state if it's a state machine
    typedef typename mp11::mp_eval_if_c<
        !has_state_machine_tag<Composite>::value,
        mp11::mp_list<>,
        get_transition_table_t,
        Composite
        > org_table;

    // and for every substate, recursively get the transition table if it's a state machine
    using submachines = mp11::mp_copy_if<generate_state_set<Composite>, has_state_machine_tag>;
    template<typename V, typename T>
    using append_recursive_transition_table = mp11::mp_append<
        V,
        typename recursive_get_transition_table<T>::type
        >;
    typedef typename mp11::mp_fold<
        submachines,
        org_table,
        append_recursive_transition_table> type;
};

}

#endif // BOOST_MSM_BACKMP11_TRANSITION_TABLE_HPP
