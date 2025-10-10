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

#ifndef BOOST_MSM_BACKMP11_METAFUNCTIONS_H
#define BOOST_MSM_BACKMP11_METAFUNCTIONS_H

#include <boost/mp11.hpp>
#include <boost/mp11/mpl_list.hpp>
#include <boost/mpl/count_if.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/msm/row_tags.hpp>

// mpl_graph graph implementation and depth first search
#include <boost/msm/mpl_graph/incidence_list_graph.hpp>
#include <boost/msm/mpl_graph/depth_first_search.hpp>

#include <boost/msm/back/traits.hpp>
#include <boost/msm/back/default_compile_policy.hpp>

namespace boost { namespace msm { namespace backmp11
{

using back::favor_runtime_speed;

template <typename Sequence, typename Range>
struct set_insert_range
{
    typedef typename ::boost::mpl::fold<
        Range,Sequence, 
        ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::placeholders::_2 >
    >::type type;
};

// returns the current state type of a transition
template <class Transition>
struct transition_source_type
{
    typedef typename Transition::current_state_type type;
};

// returns the target state type of a transition
template <class Transition>
struct transition_target_type
{
    typedef typename Transition::next_state_type type;
};

// Helper to convert a MPL sequence to Mp11
template<typename T>
struct to_mp_list
{
    typedef typename mpl::copy<T, mpl::back_inserter<mp11::mp_list<>>>::type type;
};
template<typename ...T>
struct to_mp_list<mp11::mp_list<T...>>
{
    typedef mp11::mp_list<T...> type;
};
template<typename ...T>
using to_mp_list_t = typename to_mp_list<T...>::type;

template <class stt>
struct generate_state_set;

template <class Fsm>
struct get_active_state_switch_policy_helper
{
    typedef typename Fsm::active_state_switch_policy type;
};
template <class Iter>
struct get_active_state_switch_policy_helper2
{
    typedef typename boost::mpl::deref<Iter>::type Fsm;
    typedef typename Fsm::active_state_switch_policy type;
};
// returns the active state switching policy
template <class Fsm>
struct get_active_state_switch_policy
{
    typedef typename ::boost::mpl::find_if<
        typename Fsm::configuration,
        has_active_state_switch_policy< ::boost::mpl::placeholders::_1 > >::type iter;

    typedef typename ::boost::mpl::eval_if<
        typename ::boost::is_same<
            iter, 
            typename ::boost::mpl::end<typename Fsm::configuration>::type
        >::type,
        get_active_state_switch_policy_helper<Fsm>,
        get_active_state_switch_policy_helper2< iter >
    >::type type;
};

// returns a mpl::vector containing the init states of a state machine
template <class States>
struct get_initial_states 
{
    typedef typename ::boost::mpl::if_<
        ::boost::mpl::is_sequence<States>,
        States,
        typename ::boost::mpl::push_back< ::boost::mpl::vector0<>,States>::type >::type type;
};
// returns a mpl::int_ containing the size of a region. If the argument is not a sequence, returns 1
template <class region>
struct get_number_of_regions 
{
    typedef typename mpl::if_<
        ::boost::mpl::is_sequence<region>,
        ::boost::mpl::size<region>,
        ::boost::mpl::int_<1> >::type type;
};

// builds a mpl::vector of initial states
//TODO remove duplicate from get_initial_states
template <class region>
struct get_regions_as_sequence 
{
    typedef typename ::boost::mpl::if_<
        ::boost::mpl::is_sequence<region>,
        region,
        typename ::boost::mpl::push_back< ::boost::mpl::vector0<>,region>::type >::type type;
};

template <class ToCreateSeq>
struct get_explicit_creation_as_sequence 
{
    typedef typename ::boost::mpl::if_<
        ::boost::mpl::is_sequence<ToCreateSeq>,
        ToCreateSeq,
        typename ::boost::mpl::push_back< ::boost::mpl::vector0<>,ToCreateSeq>::type >::type type;
};

// returns true for composite states
template <class State>
struct is_composite_state
{
    enum {value = has_composite_tag<State>::type::value};
    typedef typename has_composite_tag<State>::type type;
};

// iterates through a transition table to generate an ordered state set
// first the source states, transition up to down
// then the target states, up to down
template <class stt>
struct generate_state_set
{
    typedef typename to_mp_list<stt>::type stt_mp11;
    // first add the source states
    template<typename V, typename T>
    using set_push_source_state = mp11::mp_set_push_back<
        V,
        typename T::current_state_type
        >;
    typedef typename mp11::mp_fold<
        typename to_mp_list<stt>::type,
        mp11::mp_list<>,
        set_push_source_state
        > source_state_set_mp11;
    // then add the target states
    template<typename V, typename T>
    using set_push_target_state = mp11::mp_set_push_back<
        V,
        typename T::next_state_type
        >;
    typedef typename mp11::mp_fold<
        stt_mp11,
        source_state_set_mp11,
        set_push_target_state
        > state_set_mp11;
};

// extends a state set to a map with key=state and value=id
template <class stt>
struct generate_state_map
{
    typedef typename generate_state_set<stt>::state_set_mp11 state_set;
    typedef mp11::mp_iota<mp11::mp_size<state_set>> indices;
    typedef mp11::mp_transform_q<
        mp11::mp_bind<mp11::mp_list, mp11::_1, mp11::_2>,
        state_set,
        indices
        > type;
};

// filters the state set to contain only composite states
template <class stt>
struct generate_composite_state_set
{
    typedef typename generate_state_set<stt>::state_set_mp11 state_set;
    template<typename State>
    using is_composite = typename is_composite_state<State>::type;
    typedef mp11::mp_copy_if<
        state_set,
        is_composite
        > type;
};

// returns the id of a given state
template <class stt,class State>
struct get_state_id
{
    typedef mp11::mp_second<mp11::mp_map_find<
        typename generate_state_map<stt>::type,
        State
        >> type;
    
    static constexpr typename type::value_type value = type::value;
};

// iterates through the transition table and generate a set containing all the events
template <class stt>
struct generate_event_set
{
    typedef typename to_mp_list<stt>::type stt_mp11;
    template<typename V, typename T>
    using event_set_pusher = mp11::mp_set_push_back<
        V,
        typename T::transition_event
        >;
    typedef mp11::mp_fold<
        typename to_mp_list<stt>::type,
        mp11::mp_list<>,
        event_set_pusher
        > event_set_mp11;
};

// extends an event set to a map with key=event and value=id
template <class stt>
struct generate_event_map
{
    typedef typename generate_event_set<stt>::event_set_mp11 event_set;
    typedef mp11::mp_iota<mp11::mp_size<event_set>> indices;
    typedef mp11::mp_transform_q<
        mp11::mp_bind<mp11::mp_list, mp11::_1, mp11::_2>,
        event_set,
        indices
        > type;
};

// returns the id of a given event
template <class stt,class Event>
struct get_event_id
{
    typedef mp11::mp_second<mp11::mp_map_find<
        typename generate_event_map<stt>::type,
        Event
        >> type;
    enum {value = type::value};
};

// returns a mpl::bool_<true> if State has Event as deferred event
template <class State, class Event>
struct has_state_delayed_event
{
    typedef typename mp11::mp_contains<
        typename to_mp_list<typename State::deferred_events>::type,
        Event
        > type;
};
// returns a mpl::bool_<true> if State has any deferred event
template <class State>
struct has_state_delayed_events  
{
    typedef typename mp11::mp_not<mp11::mp_empty<
        typename to_mp_list<typename State::deferred_events>::type
        >> type;
};

// Template used to create dummy entries for initial states not found in the stt.
template< typename T1 >
struct not_a_row
{
    typedef int not_real_row_tag;
    struct dummy_event 
    {
    };
    typedef T1                  current_state_type;
    typedef T1                  next_state_type;
    typedef dummy_event         transition_event;
};

// metafunctions used to find out if a state is entry, exit or something else
template <class State>
struct is_pseudo_entry 
{
    typedef typename ::boost::mpl::if_< typename has_pseudo_entry<State>::type,
        ::boost::mpl::bool_<true>,::boost::mpl::bool_<false> 
    >::type type;
};
// says if a state is an exit pseudo state
template <class State>
struct is_pseudo_exit 
{
    typedef typename has_pseudo_exit<State>::type type;
    static constexpr bool value = type::value;
};
// says if a state is an entry pseudo state or an explicit entry
template <class State>
struct is_direct_entry 
{
    typedef typename ::boost::mpl::if_< typename has_explicit_entry_state<State>::type,
        ::boost::mpl::bool_<true>, ::boost::mpl::bool_<false> 
    >::type type;
};

//converts a "fake" (simulated in a state_machine_ description )state into one which will really get created
template <class StateType,class CompositeType>
struct convert_fake_state
{
    // converts a state (explicit entry) into the state we really are going to create (explicit<>)
    typedef typename ::boost::mpl::if_<
        typename is_direct_entry<StateType>::type,
        typename CompositeType::template direct<StateType>,
        typename ::boost::mpl::identity<StateType>::type
    >::type type;
};

template <class StateType>
struct get_explicit_creation 
{
    typedef typename StateType::explicit_creation type;
};

template <class StateType>
struct get_wrapped_entry 
{
    typedef typename StateType::wrapped_entry type;
};
// used for states created with explicit_creation
// if the state is an explicit entry, we reach for the wrapped state
// otherwise, this returns the state itself
template <class StateType>
struct get_wrapped_state 
{
    typedef typename ::boost::mpl::eval_if<
                typename has_wrapped_entry<StateType>::type,
                get_wrapped_entry<StateType>,
                ::boost::mpl::identity<StateType> >::type type;
};

template <class Derived>
struct create_stt 
{
    //typedef typename Derived::transition_table stt;
    typedef typename Derived::real_transition_table Stt;
    // get the state set
    typedef typename generate_state_set<Stt>::state_set_mp11 states;
    // transform the initial region(s) in a sequence
    typedef typename get_regions_as_sequence<typename Derived::initial_state>::type init_states;
    // iterate through the initial states and add them in the stt if not already there
    template<typename V, typename T>
    using states_pusher = mp11::mp_if_c<
        mp11::mp_set_contains<states, T>::value,
        V,
        mp11::mp_push_back<
            V,
            not_a_row<typename get_wrapped_state<T>::type>
            >
        >;
    typedef typename mp11::mp_fold<
        typename to_mp_list<init_states>::type,
        typename to_mp_list<Stt>::type,
        states_pusher
        > with_init;

    // do the same for states marked as explicitly created
    typedef typename get_explicit_creation_as_sequence<
       typename ::boost::mpl::eval_if<
            typename has_explicit_creation<Derived>::type,
            get_explicit_creation<Derived>,
            ::boost::mpl::vector0<> >::type
        >::type fake_explicit_created;

    typedef typename 
        ::boost::mpl::transform<
        fake_explicit_created,convert_fake_state< ::boost::mpl::placeholders::_1,Derived> >::type explicit_created;

    typedef typename mp11::mp_fold<
        typename to_mp_list<explicit_created>::type,
        with_init,
        states_pusher
        > type;
};

// returns the transition table of a Composite state
template <class Composite>
struct get_transition_table
{
    typedef typename create_stt<Composite>::type type;
};

// recursively builds an internal table including those of substates, sub-substates etc.
// variant for submachines
template <class StateType,class IsComposite>
struct recursive_get_internal_transition_table
{
    // get the composite's internal table
    typedef typename StateType::internal_transition_table composite_table;
    // and for every substate (state of submachine), recursively get the internal transition table
    typedef typename generate_state_set<typename StateType::stt>::state_set_mp11 composite_states;
    template<typename V, typename T>
    using append_recursive_internal_transition_table = mp11::mp_append<
        V,
        typename recursive_get_internal_transition_table<T, typename has_composite_tag<T>::type>::type
        >;
    typedef typename mp11::mp_fold<
        composite_states,
        typename to_mp_list<composite_table>::type,
        append_recursive_internal_transition_table
        > type;
};
// stop iterating on leafs (simple states)
template <class StateType>
struct recursive_get_internal_transition_table<StateType, ::boost::mpl::false_ >
{
    typedef typename to_mp_list<
        typename StateType::internal_transition_table
        >::type type;
};
// recursively get a transition table for a given composite state.
// returns the transition table for this state + the tables of all composite sub states recursively
template <class Composite>
struct recursive_get_transition_table
{
    // get the transition table of the state if it's a state machine
    template<typename T>
    using get_transition_table_mp11 = typename get_transition_table<T>::type;
    typedef typename mp11::mp_eval_if_c<
        !has_composite_tag<Composite>::type::value,
        mp11::mp_list<>,
        get_transition_table_mp11,
        Composite
        > org_table;

    typedef typename generate_state_set<org_table>::state_set_mp11 states;

    // and for every substate, recursively get the transition table if it's a state machine
    template<typename V, typename T>
    using append_recursive_transition_table = mp11::mp_append<
        V,
        typename recursive_get_transition_table<T>::type
        >;
    typedef typename mp11::mp_fold<
        states,
        org_table,
        append_recursive_transition_table> type;
};

// metafunction used to say if a SM has pseudo exit states
template <class Derived>
struct has_fsm_deferred_events 
{
    typedef typename create_stt<Derived>::type Stt;
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;

    template<typename T>
    using has_activate_deferred_events_mp11 = typename has_activate_deferred_events<T>::type;
    template<typename T>
    using has_state_delayed_events_mp11 = typename has_state_delayed_events<T>::type;
    typedef typename mp11::mp_or<
        typename has_activate_deferred_events<Derived>::type,
        mp11::mp_any_of<
            typename to_mp_list<typename Derived::configuration>::type,
            has_activate_deferred_events_mp11
            >,
        mp11::mp_any_of<
            state_set_mp11,
            has_state_delayed_events_mp11
            >
        > type;
};

struct favor_compile_time;

// returns a mpl::bool_<true> if State has any delayed event
template <class Event, class CompilePolicy>
struct is_completion_event;
template <class Event>
struct is_completion_event<Event, favor_runtime_speed>
{
    typedef typename ::boost::mpl::if_<
        has_completion_event<Event>,
        ::boost::mpl::bool_<true>,
        ::boost::mpl::bool_<false> >::type type;

    static constexpr bool value(const Event&)
    {
        return type::value;
    }
};

// metafunction used to say if a SM has eventless transitions
template <class Derived>
struct has_fsm_eventless_transition 
{
    typedef typename create_stt<Derived>::type Stt;
    typedef typename generate_event_set<Stt>::event_set_mp11 event_list;

    typedef mp11::mp_any_of<event_list, has_completion_event> type;
};
template <class Derived>
struct find_completion_events 
{
    typedef typename create_stt<Derived>::type Stt;
    typedef typename generate_event_set<Stt>::event_set_mp11 event_list;

    template<typename T>
    using has_completion_event_mp11 = typename has_completion_event<T>::type;
    typedef typename mp11::mp_copy_if<
        event_list,
        has_completion_event_mp11
        > type;
};

template <class Transition>
struct make_vector 
{
    typedef ::boost::mpl::vector<Transition> type;
};
template< typename Entry > 
struct get_first_element_pair_second
{ 
    typedef typename ::boost::mpl::front<typename Entry::second>::type type;
}; 

 //returns the owner of an explicit_entry state
 //which is the containing SM if the transition originates from outside the containing SM
 //or else the explicit_entry state itself
template <class State,class ContainingSM>
struct get_owner 
{
    typedef typename ::boost::mpl::if_<
        typename ::boost::mpl::not_<typename ::boost::is_same<typename State::owner,
                                                              ContainingSM >::type>::type,
        typename State::owner, 
        State >::type type;
};

template <class Sequence,class ContainingSM>
struct get_fork_owner 
{
    typedef typename ::boost::mpl::front<Sequence>::type seq_front;
    typedef typename ::boost::mpl::if_<
                    typename ::boost::mpl::not_<
                        typename ::boost::is_same<typename seq_front::owner,ContainingSM>::type>::type,
                    typename seq_front::owner, 
                    seq_front >::type type;
};

template <class StateType,class ContainingSM>
struct make_exit 
{
    typedef typename ::boost::mpl::if_<
             typename is_pseudo_exit<StateType>::type ,
             typename ContainingSM::template exit_pt<StateType>,
             typename ::boost::mpl::identity<StateType>::type
            >::type type;
};

template <class StateType,class ContainingSM>
struct make_entry 
{
    typedef typename ::boost::mpl::if_<
        typename is_pseudo_entry<StateType>::type ,
        typename ContainingSM::template entry_pt<StateType>,
        typename ::boost::mpl::if_<
                typename is_direct_entry<StateType>::type,
                typename ContainingSM::template direct<StateType>,
                typename ::boost::mpl::identity<StateType>::type
                >::type
        >::type type;
};
// metafunction used to say if a SM has pseudo exit states
template <class StateType>
struct has_exit_pseudo_states_helper 
{
    typedef typename StateType::stt Stt;
    typedef typename generate_state_set<Stt>::type state_list;

    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
                state_list,is_pseudo_exit< ::boost::mpl::placeholders::_1> >::value != 0> type;
};
template <class StateType>
struct has_exit_pseudo_states 
{
    typedef typename ::boost::mpl::eval_if<typename is_composite_state<StateType>::type,
        has_exit_pseudo_states_helper<StateType>,
        ::boost::mpl::bool_<false> >::type type;
};

// builds flags (add internal_flag_list and flag_list). internal_flag_list is used for terminate/interrupt states
template <class StateType>
struct get_flag_list
{
    typedef typename mp11::mp_append<
        typename to_mp_list<typename StateType::flag_list>::type,
        typename to_mp_list<typename StateType::internal_flag_list>::type
        > type;
};

template <class StateType>
struct is_state_blocking 
{
    template<typename T>
    using has_event_blocking_flag_mp11 = typename has_event_blocking_flag<T>::type;
    typedef typename mp11::mp_any_of<
        typename get_flag_list<StateType>::type,
        has_event_blocking_flag_mp11
        > type;

};
// returns a mpl::bool_<true> if fsm has an event blocking flag in one of its substates
template <class StateType>
struct has_fsm_blocking_states  
{
    typedef typename create_stt<StateType>::type Stt;
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;

    template<typename T>
    using is_state_blocking_mp11 = typename is_state_blocking<T>::type;
    typedef typename mp11::mp_any_of<
        state_set_mp11,
        is_state_blocking_mp11
        > type;
};

template <class StateType>
struct is_no_exception_thrown
{
    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
        typename StateType::configuration,
        has_no_exception_thrown< ::boost::mpl::placeholders::_1 > >::value != 0> found;

    typedef typename ::boost::mpl::or_<
        typename has_no_exception_thrown<StateType>::type,
        found
    >::type type;

    static constexpr typename type::value_type value = type::value;
};

template <class StateType>
struct is_no_message_queue
{
    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
        typename StateType::configuration,
        has_no_message_queue< ::boost::mpl::placeholders::_1 > >::value != 0> found;

    typedef typename ::boost::mpl::or_<
        typename has_no_message_queue<StateType>::type,
        found
    >::type type;
};

template <class StateType>
struct is_active_state_switch_policy 
{
    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
        typename StateType::configuration,
        has_active_state_switch_policy< ::boost::mpl::placeholders::_1 > >::value != 0> found;

    typedef typename ::boost::mpl::or_<
        typename has_active_state_switch_policy<StateType>::type,
        found
    >::type type;
};

template <class StateType>
struct get_initial_event 
{
    typedef typename StateType::initial_event type;
};

template <class StateType>
struct get_final_event 
{
    typedef typename StateType::final_event type;
};

template <class TransitionTable, class InitState>
struct build_one_orthogonal_region 
{
     template<typename Row>
     struct row_to_incidence :
         mp11::mp_list<
                ::boost::mpl::pair<
                    typename Row::next_state_type, 
                    typename Row::transition_event>, 
                typename Row::current_state_type, 
                typename Row::next_state_type
         > {};

     typedef typename mp11::mp_transform<
        row_to_incidence,
        typename to_mp_list<TransitionTable>::type
        > transition_incidence_list;

     typedef ::boost::msm::mpl_graph::incidence_list_graph<transition_incidence_list>
         transition_graph;

     struct preordering_dfs_visitor : 
         ::boost::msm::mpl_graph::dfs_default_visitor_operations 
     {    
         template<typename Node, typename Graph, typename State>
         struct discover_vertex :
             ::boost::mpl::insert<State, Node>
         {};
     };

     typedef typename mpl::first< 
         typename ::boost::msm::mpl_graph::depth_first_search<
            transition_graph, 
            preordering_dfs_visitor,
            ::boost::mpl::set<>,
            InitState
         >::type
     >::type type;
};

template <class Fsm>
struct find_entry_states 
{
    typedef mp11::mp_copy_if<
        typename Fsm::substate_list,
        has_explicit_entry_state
        > type;
};

template <class Set1, class Set2>
struct is_common_element 
{
    typedef typename ::boost::mpl::fold<
        Set1, ::boost::mpl::false_,
        ::boost::mpl::if_<
            ::boost::mpl::has_key<
                Set2,
                ::boost::mpl::placeholders::_2
            >,
            ::boost::mpl::true_,
            ::boost::mpl::placeholders::_1
        >
    >::type type;
};

template <class EntryRegion, class AllRegions>
struct add_entry_region 
{
    typedef typename ::boost::mpl::transform<
        AllRegions, 
        ::boost::mpl::if_<
            is_common_element<EntryRegion, ::boost::mpl::placeholders::_1>,
            set_insert_range< ::boost::mpl::placeholders::_1, EntryRegion>,
            ::boost::mpl::placeholders::_1
        >
    >::type type;
};

// build a vector of regions states (as a set)
// one set of states for every region
template <class Fsm, class InitStates>
struct build_orthogonal_regions 
{
    typedef typename 
        ::boost::mpl::fold<
            InitStates, ::boost::mpl::vector0<>,
            ::boost::mpl::push_back< 
                ::boost::mpl::placeholders::_1, 
                build_one_orthogonal_region< typename Fsm::stt, ::boost::mpl::placeholders::_2 > >
        >::type without_entries;

    typedef typename 
        ::boost::mpl::fold<
        typename find_entry_states<Fsm>::type, ::boost::mpl::vector0<>,
            ::boost::mpl::push_back< 
                ::boost::mpl::placeholders::_1, 
                build_one_orthogonal_region< typename Fsm::stt, ::boost::mpl::placeholders::_2 > >
        >::type only_entries;

    typedef typename ::boost::mpl::fold<
        only_entries , without_entries,
        add_entry_region< ::boost::mpl::placeholders::_2, ::boost::mpl::placeholders::_1>
    >::type type;
};

template <class GraphAsSeqOfSets, class StateType>
struct find_region_index
{
    typedef typename 
        ::boost::mpl::fold<
            GraphAsSeqOfSets, ::boost::mpl::pair< ::boost::mpl::int_< -1 > /*res*/, ::boost::mpl::int_<0> /*counter*/ >,
            ::boost::mpl::if_<
                ::boost::mpl::has_key< ::boost::mpl::placeholders::_2, StateType >,
                ::boost::mpl::pair< 
                    ::boost::mpl::second< ::boost::mpl::placeholders::_1 >,
                    ::boost::mpl::next< ::boost::mpl::second< ::boost::mpl::placeholders::_1 > >
                >,
                ::boost::mpl::pair< 
                    ::boost::mpl::first< ::boost::mpl::placeholders::_1 >,
                    ::boost::mpl::next< ::boost::mpl::second< ::boost::mpl::placeholders::_1 > >
                >
            >
        >::type result_pair;
    typedef typename ::boost::mpl::first<result_pair>::type type;
    enum {value = type::value};
};

// helper to find out if a SM has an active exit state and is therefore waiting for exiting
template <class StateType,class OwnerFct,class FSM>
inline
typename ::boost::enable_if<typename ::boost::mpl::and_<typename is_composite_state<FSM>::type,
                                                        typename is_pseudo_exit<StateType>::type>,bool >::type
is_exit_state_active(FSM& fsm)
{
    typedef typename OwnerFct::type Composite;
    Composite& comp = fsm.template get_state<Composite&>();
    const int state_id = comp.template get_state_id<StateType>();
    for (const auto active_state_id : comp.get_active_state_ids())
    {
        if (active_state_id == state_id)
        {
            return true;
        }
    }
    return false;
}
template <class StateType,class OwnerFct,class FSM>
inline
typename ::boost::disable_if<typename ::boost::mpl::and_<typename is_composite_state<FSM>::type,
                                                         typename is_pseudo_exit<StateType>::type>,bool >::type
is_exit_state_active(FSM&)
{
    return false;
}

// transformation metafunction to end interrupt flags
template <class Event>
struct transform_to_end_interrupt 
{
    typedef boost::msm::EndInterruptFlag<Event> type;
};
// transform a sequence of events into another one of EndInterruptFlag<Event>
template <class Events>
struct apply_end_interrupt_flag 
{
    typedef typename 
        ::boost::mpl::transform<
        Events,transform_to_end_interrupt< ::boost::mpl::placeholders::_1> >::type type;
};
// returns a mpl vector containing all end interrupt events if sequence, otherwise the same event
template <class Event>
struct get_interrupt_events 
{
    typedef typename ::boost::mpl::eval_if<
        ::boost::mpl::is_sequence<Event>,
        apply_end_interrupt_flag<Event>,
        boost::mpl::vector1<boost::msm::EndInterruptFlag<Event> > >::type type;
};

template <class Events>
struct build_interrupt_state_flag_list
{
    typedef ::boost::mpl::vector<boost::msm::InterruptedFlag> first_part;
    typedef typename ::boost::mpl::insert_range< 
        first_part, 
        typename ::boost::mpl::end< first_part >::type,
        Events
    >::type type;
};

}}} // boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_METAFUNCTIONS_H
