// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACK_FAVOR_COMPILE_TIME_H
#define BOOST_MSM_BACK_FAVOR_COMPILE_TIME_H

#include <utility>
#include <deque>

#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/any.hpp>

#include <boost/msm/common.hpp>
#include <boost/msm/backmp11/metafunctions.hpp>
#include <boost/msm/back/common_types.hpp>
#include <boost/msm/backmp11/dispatch_table.hpp>

namespace boost { namespace msm { namespace back
{

template <class Fsm>
struct process_any_event_helper
{
    process_any_event_helper(msm::back::HandledEnum& res_,Fsm* self_,::boost::any any_event_):
    res(res_),self(self_),any_event(any_event_),finished(false){}
    
    static HandledEnum execute(Fsm* self, ::boost::any const& any_event)
    {
        typedef typename recursive_get_transition_table<Fsm>::type stt;
        typedef typename generate_event_set<stt>::event_set_mp11 stt_events;
        typedef typename recursive_get_internal_transition_table<Fsm, ::boost::mpl::true_>::type istt;
        typedef typename generate_event_set<typename Fsm::template create_real_stt<Fsm, istt>::type>::event_set_mp11 istt_events;
        typedef mp11::mp_set_union<stt_events, istt_events> all_events;
        
        HandledEnum res= HANDLED_FALSE;
        mp11::mp_for_each<all_events>(process_any_event_helper<Fsm>(res, self, any_event));
        return res;
    }
    
    template <class Event>
    void operator()(Event const&)
    {
        if (!finished && ::boost::any_cast<Event>(&any_event) != 0)
        {
            finished = true;
            res = self->process_event_internal(::boost::any_cast<Event>(any_event));
        }
    }
private:
    msm::back::HandledEnum&     res;
    Fsm*                        self;
    ::boost::any                any_event;
    bool                        finished;
};

#define BOOST_MSM_BACK_GENERATE_FSM(fsmname)                                                        \
    template<>                                                                                      \
    template<typename Policy>                                                                       \
    fsmname::state_machine(typename enable_if<is_same<Policy, favor_compile_time>>::type*)          \
        :state_machine(internal_tag{}) {}                                                           \
    template<>                                                                                      \
    ::boost::msm::back::HandledEnum fsmname::process_any_event( ::boost::any const& any_event)      \
    {                                                                                               \
        return ::boost::msm::back::process_any_event_helper<fsmname>::execute(this, any_event);     \
    }                                                                                               \
    template<>                                                                                      \
    template<class Event, class Policy>                                                             \
    typename ::boost::enable_if<                                                                    \
        boost::is_same<Policy, boost::msm::back::favor_compile_time>,                               \
        boost::msm::back::execute_return>::type                                                     \
    BOOST_NOINLINE fsmname::process_event_internal(Event const& evt, EventSource source)            \
    {                                                                                               \
        return process_event_internal_impl(evt, source);                                            \
    }



struct favor_compile_time
{
    typedef int compile_policy;
    typedef ::boost::mpl::false_ add_forwarding_rows;
};

struct chain_row
{
    template<typename Fsm, typename Event>
    HandledEnum operator()(Fsm& fsm, int region,int state,Event const& evt) const
    {
        typedef HandledEnum (*real_cell)(Fsm&, int,int,Event const&);
        HandledEnum res = HANDLED_FALSE;
        typename std::deque<generic_cell>::const_iterator it = one_state.begin();
        while (it != one_state.end() && (res != HANDLED_TRUE && res != HANDLED_DEFERRED ))
        {
            auto fnc = reinterpret_cast<real_cell>(reinterpret_cast<void*>(*it));
            HandledEnum handled = (*fnc)(fsm,region,state,evt);
            // reject is considered as erasing an error (HANDLED_FALSE)
            if ((HANDLED_FALSE==handled) && (HANDLED_GUARD_REJECT==res) )
                res = HANDLED_GUARD_REJECT;
            else
                res = handled;
            ++it;
        }
        return res;
    }
    // Use a deque with a generic type to avoid unnecessary template instantiations.
    std::deque<generic_cell> one_state;
};

template<>
struct cell_initializer<favor_compile_time>
{
    // TODO:
    // Check against default_init_cell behavior in original implementation,
    // in particular deferring transitions and internal transitions.
    // static void default_init(chain_row* entries, const generic_init_cell_value* array, size_t size)
    // {
    //     for (size_t i=0; i<size; i++)
    //     {
    //         const auto& item = array[i];
    //         entries[item.index].one_state.push_back(item.address);
    //     }
    // }

    static void init(chain_row* entries, const generic_init_cell_value* array, size_t size)
    {
        for (size_t i=0; i<size; i++)
        {
            const auto& item = array[i];
            entries[item.index].one_state.push_front(item.address);
        }
    }
};

// split the stt into rows grouped by event
template<typename Stt>
struct group_rows_by_event
{
    typedef typename generate_event_set<Stt>::event_set_mp11 event_set_mp11;
    // Consider only real rows
    template<typename Transition>
    using is_real_row = mp11::mp_not<typename has_not_real_row_tag<Transition>::type>;
    typedef mp11::mp_copy_if<Stt, is_real_row> real_rows;
    // Go through each row and group transitions by event.
    template<typename Row, typename Event>
    using has_event = std::is_same<typename Row::transition_event, Event>;
    template<typename V, typename Event>
    using partition_by_event = mp11::mp_list<
        // From first element we remove the rows that we have grouped
        mp11::mp_remove_if_q<
            mp11::mp_first<V>,
            mp11::mp_bind_back<has_event, Event>
            >,
        // To second element we append a group of rows that is handled by the event
        mp11::mp_push_back<
            mp11::mp_second<V>,
            mp11::mp_copy_if_q<
                mp11::mp_first<V>,
                mp11::mp_bind_back<has_event, Event>
                >
            >
        >;
    typedef mp11::mp_fold<
        event_set_mp11,
        mp11::mp_list<real_rows, mp11::mp_list<>>,
        partition_by_event
        > grouped_rows;
    static_assert(mp11::mp_empty<mp11::mp_first<grouped_rows>>::value, "Internal error: Not all rows grouped");
    typedef mp11::mp_second<grouped_rows> type;
};

template<typename Stt, typename Event>
using get_rows_of_event = mp11::mp_at_c<
    typename group_rows_by_event<Stt>::type,
    get_event_id<Stt, Event>::value
    >;

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the SM take its transition on the given
// Event type.
template <class Fsm,class Stt, class Event>
struct dispatch_table < Fsm, Stt, Event, ::boost::msm::back::favor_compile_time>
{
 private:
    // This is a table of these function pointers.
    typedef HandledEnum (*cell)(Fsm&, int,int,Event const&);

    // Compute the maximum state value in the sm so we know how big
    // to make the table
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;
    BOOST_STATIC_CONSTANT(int, max_state = (mp11::mp_size<state_set_mp11>::value));

    typedef typename generate_event_set<Stt>::event_set_mp11 event_set;

    template <class TransitionState>
    static HandledEnum call_submachine(Fsm& fsm, int , int , Event const& evt)
    {
        return (fsm.template get_state<TransitionState&>()).process_event_internal(evt);
    }

    using init_cell_value = init_cell_value<cell>;

    template<size_t v1, cell v2>
    using init_cell_constant = init_cell_constant<v1, cell, v2>;

    template<cell v>
    using cell_constant = std::integral_constant<cell, v>;

    using cell_initializer = cell_initializer<favor_compile_time>;

    // Helpers for state processing
    template<typename SubFsm>
    struct call_submachine_cell
    {
        using type = cell_constant<&call_submachine<SubFsm>>;
    };
    template<typename State>
    using defer_transition_cell = cell_constant<&State::defer_transition>;
    template <typename State>
    using preprocess_state = init_cell_constant<
        get_table_index<Fsm, State, Event>::value,
        mp11::mp_eval_if_c<
            has_state_delayed_event<State, Event>::type::value,
            mp11::mp_defer<defer_transition_cell, State>,
            call_submachine_cell,
            State
            >::type::value
        >;
    template<typename State>
    using state_filter_predicate = mp11::mp_or<
        typename has_state_delayed_event<State, Event>::type,
        mp11::mp_and<is_composite_state<State>, mp11::mp_not<is_same<State, Fsm>>>
        >;

    // Helpers for row processing
    template<typename Transition>
    using preprocess_row = init_cell_constant<
        // Offset into the entries array
        get_table_index<Fsm, typename Transition::current_state_type>::value,
        // Address of the execute function
        &Transition::execute
        >;

 public:
    // initialize the dispatch table for a given Event and Fsm
    template<typename EventType = Event>
    dispatch_table(typename enable_if<mp11::mp_set_contains<event_set, EventType>>::type* = 0)
    {
        // Fill in cells for matching transitions
        typedef mp11::mp_transform<
            preprocess_row,
            get_rows_of_event<Stt, Event>
            > preprocessed_rows;
        cell_initializer::init(
            entries,
            get_init_cells<cell, preprocessed_rows>(),
            mp11::mp_size<preprocessed_rows>::value
            );

        // Initialize cells that defer an event or call a submachine.
        // This needs to happen last to make sure calling a submachine is handled before
        // trying to call the transitions defined in this machine.
        typedef mp11::mp_copy_if<
            typename generate_state_set<Stt>::state_set_mp11,
            state_filter_predicate
            > filtered_states;
        typedef mp11::mp_transform<
            preprocess_state,
            filtered_states
            > preprocessed_states;
        cell_initializer::init(
            entries,
            get_init_cells<cell, preprocessed_states>(),
            mp11::mp_size<preprocessed_states>::value
            );
    }

    // shortened version in case the stt doesn't contain the event,
    // it can only be handled by a submachine
    template<typename EventType = Event>
    dispatch_table(typename disable_if<mp11::mp_set_contains<event_set, EventType>>::type* = 0)
    {
        // Initialize cells that defer an event or call a submachine.
        typedef mp11::mp_copy_if<
            typename generate_composite_state_set<Stt>::type,
            state_filter_predicate
            > filtered_states;
        typedef mp11::mp_transform<
            preprocess_state,
            filtered_states
            > preprocessed_states;
        cell_initializer::init(
            entries,
            get_init_cells<cell, preprocessed_states>(),
            mp11::mp_size<preprocessed_states>::value
            );
    }

    // The singleton instance.
    static const dispatch_table& instance() {
        static dispatch_table table;
        return table;
    }

 public: // data members
     chain_row entries[max_state+1];
};

}}} // boost::msm::back

#endif //BOOST_MSM_BACK_FAVOR_COMPILE_TIME_H
