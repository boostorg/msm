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
    template <class Event>
    void operator()(boost::msm::wrap<Event> const&)
    {
        if ( ! finished && ::boost::any_cast<Event>(&any_event)!=0)
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

#define BOOST_MSM_BACK_GENERATE_PROCESS_EVENT(fsmname)                                              \
    namespace boost { namespace msm { namespace back{                                               \
    template<>                                                                                      \
    ::boost::msm::back::HandledEnum fsmname::process_any_event( ::boost::any const& any_event)      \
    {                                                                                               \
        typedef ::boost::msm::back::recursive_get_transition_table<fsmname>::type stt;              \
        typedef ::boost::msm::back::generate_event_set<stt>::type stt_events;                       \
        typedef ::boost::msm::back::recursive_get_internal_transition_table<fsmname, ::boost::mpl::true_ >::type istt;    \
        typedef ::boost::msm::back::generate_event_set<create_real_stt<fsmname,istt>::type >::type istt_events;  \
        typedef ::boost::msm::back::set_insert_range<stt_events,istt_events>::type all_events;      \
        ::boost::msm::back::HandledEnum res= ::boost::msm::back::HANDLED_FALSE;                     \
        ::boost::mpl::for_each<all_events, ::boost::msm::wrap< ::boost::mpl::placeholders::_1> >    \
        (::boost::msm::back::process_any_event_helper<fsmname>(res,this,any_event));                \
        return res;                                                                                 \
    }                                                                                               \
    }}}

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
        typedef HandledEnum (*cell)(Fsm&, int,int,Event const&);
        HandledEnum res = HANDLED_FALSE;
        typename std::deque<void*>::const_iterator it = one_state.begin();
        while (it != one_state.end() && (res != HANDLED_TRUE && res != HANDLED_DEFERRED ))
        {
            auto fnc = reinterpret_cast<cell>(*it);
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
    // Use a deque with void* to avoid unnecessary template instantiations.
    std::deque<void*> one_state;
};

// A function object for use with mpl::for_each that stuffs
// transitions into cells.
template<>
struct init_cell<favor_compile_time>
{
    init_cell(chain_row* entries_) : entries(entries_) {}

    template<typename PreprocessedRow>
    void operator()(PreprocessedRow const&)
    {
        entries[mp11::mp_first<PreprocessedRow>::value].one_state.push_front(reinterpret_cast<void*>(mp11::mp_second<PreprocessedRow>::value));
    }

    chain_row* entries;
};

template<>
struct default_init_cell<favor_compile_time>
{
    default_init_cell(chain_row* entries_) : entries(entries_) {}

    template<typename PreprocessedRow>
    void operator()(PreprocessedRow const&)
    {
        entries[mp11::mp_first<PreprocessedRow>::value].one_state.push_back(reinterpret_cast<void*>(mp11::mp_second<PreprocessedRow>::value));
    }

    chain_row* entries;
};


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

    template <class TransitionState>
    static HandledEnum call_submachine(Fsm& fsm, int , int , Event const& evt)
    {
        return (fsm.template get_state<TransitionState&>()).process_any_event( ::boost::any(evt));
    }

    // Helpers for state processing
    template<typename fsm = Fsm>
    using fsm_defer_transition = std::integral_constant<
        cell,
        &fsm::defer_transition
        >;
    template<typename State>
    using fsm_call_submachine = std::integral_constant<
        cell,
        &call_submachine<State>
        >;
    template<typename State>
    using preprocess_state = mp11::mp_list<
        // Offset into the entries array
        get_table_index<Fsm, State, Event>,
        // Address of the function to assign
        mp11::mp_if_c<
            is_completion_event<Event>::type::value,
            // Completion event
            std::integral_constant<
                cell,
                &Fsm::default_eventless_transition
                >,
            // No completion event
            mp11::mp_eval_if_c<
                !has_state_delayed_event<State, Event>::type::value,
                // Not a deferred event
                mp11::mp_if_c<
                    is_same<State, Fsm>::value,
                    // State is this Fsm
                    std::integral_constant<
                        cell,
                        &Fsm::call_no_transition
                        >,
                    // State is not this Fsm
                    mp11::mp_eval_if_c<
                        !is_composite_state<State>::type::value,
                        // State is not a composite
                        std::integral_constant<
                            cell,
                            &Fsm::call_no_transition
                            >,
                        // State is a composite
                        fsm_call_submachine,
                        State
                        >
                    >,
                // A deferred event
                fsm_defer_transition,
                Fsm
                >
            >
        >;
    typedef mp11::mp_transform<
        preprocess_state,
        typename generate_state_set<Stt>::state_set_mp11
        > preprocessed_states;

    // Helpers for row processing
    template <typename T>
    using event_filter_predicate = mp11::mp_and<
        is_base_of<typename T::transition_event, Event>,
        mp11::mp_not<typename has_not_real_row_tag<T>::type>
        >;
    typedef mp11::mp_copy_if<
        typename to_mp_list<Stt>::type,
        event_filter_predicate
        > filtered_rows;
    template<typename Transition>
    using preprocess_row = mp11::mp_list<
        // Offset into the entries array
        get_table_index<Fsm, typename Transition::current_state_type>,
        // Address of the execute function
        std::integral_constant<
            cell,
            &Transition::execute
            >
        >;
    typedef mp11::mp_transform<
        preprocess_row,
        filtered_rows
        > preprocessed_rows;

 public:
    // initialize the dispatch table for a given Event and Fsm
    dispatch_table()
    {
        using default_init_cell = default_init_cell<favor_compile_time>;
        using init_cell = init_cell<favor_compile_time>;

        // Initialize cells for no transition.
        mp11::mp_for_each<preprocessed_states>(default_init_cell{entries});

        // Fill in cells for matching transitions
        mp11::mp_for_each<preprocessed_rows>(init_cell{entries});
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
