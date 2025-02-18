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
template <typename Fsm>
struct init_cell
{
    init_cell(chain_row* entries_)
        : entries(entries_)
    {}
    // version for transition event not base of our event
    template <class Transition>
    typename ::boost::disable_if<
        typename ::boost::is_same<typename Transition::current_state_type,Fsm>::type
    ,void>::type
    init_event_base_case(Transition const&) const
    {
        typedef typename create_stt<Fsm>::type stt;
        BOOST_STATIC_CONSTANT(int, state_id =
            (get_state_id<stt,typename Transition::current_state_type>::value));
        entries[state_id+1].one_state.push_front(reinterpret_cast<void*>(&Transition::execute));
    }
    template <class Transition>
    typename ::boost::enable_if<
        typename ::boost::is_same<typename Transition::current_state_type,Fsm>::type
    ,void>::type
    init_event_base_case(Transition const&) const
    {
        entries[0].one_state.push_front(reinterpret_cast<void*>(&Transition::execute));
    }

    // Cell initializer function object, used with mpl::for_each
    template <class Transition>
    typename ::boost::enable_if<typename has_not_real_row_tag<Transition>::type,void >::type
        operator()(Transition const&,boost::msm::back::dummy<0> = 0) const
    {
        // version for not real rows. No problem because irrelevant for process_event
    }
    template <class Transition>
    typename ::boost::disable_if<typename has_not_real_row_tag<Transition>::type,void >::type
    operator()(Transition const& tr,boost::msm::back::dummy<1> = 0) const
    {
        // only if the transition event is a base of our event is the reinterpret_case safe
        // -> which is always the case, because this functor is called with filtered rows
        init_event_base_case(tr);
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
    typedef bool (*guard)(Fsm&, Event const&);

    // Compute the maximum state value in the sm so we know how big
    // to make the table
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;
    BOOST_STATIC_CONSTANT(int, max_state = (mp11::mp_size<state_set_mp11>::value));

    using chain_row = chain_row;

    template <class TransitionState>
    static HandledEnum call_submachine(Fsm& fsm, int , int , Event const& evt)
    {
        return (fsm.template get_state<TransitionState&>()).process_any_event( ::boost::any(evt));
    }

    using init_cell = init_cell<Fsm>;

    // Cell default-initializer function object, used with mpl::for_each
    // initializes with call_no_transition, defer_transition or default_eventless_transition
    // variant for non-anonymous transitions
    template <class EventType,class Enable=void>
    struct default_init_cell
    {
        default_init_cell(dispatch_table* self_,chain_row* tofill_entries_)
            : self(self_),tofill_entries(tofill_entries_)
        {}
        template <bool deferred,bool composite, int some_dummy=0>
        struct helper
        {};
        template <int some_dummy> struct helper<true,false,some_dummy>
        {
            template <class State>
            static void execute(State const&,chain_row* tofill)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &Fsm::defer_transition;
                tofill[state_id+1].one_state.push_back(call_no_transition);
            }
        };
        template <int some_dummy> struct helper<true,true,some_dummy>
        {
            template <class State>
            static void execute(State const&,chain_row* tofill)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &Fsm::defer_transition;
                tofill[state_id+1].one_state.push_back(call_no_transition);
            }
        };
        template <int some_dummy> struct helper<false,true,some_dummy>
        {
            template <class State>
            static
            typename ::boost::enable_if<
                typename ::boost::is_same<State,Fsm>::type
            ,void>::type
            execute(State const&,chain_row* tofill,boost::msm::back::dummy<0> = 0)
            {
                // for internal tables
                cell call_no_transition_internal = &Fsm::call_no_transition;
                tofill[0].one_state.push_front(call_no_transition_internal);
            }
            template <class State>
            static
            typename ::boost::disable_if<
                typename ::boost::is_same<State,Fsm>::type
            ,void>::type
            execute(State const&,chain_row* tofill,boost::msm::back::dummy<1> = 0)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &call_submachine< State >;
                tofill[state_id+1].one_state.push_front(call_no_transition);
            }
        };
        template <int some_dummy> struct helper<false,false,some_dummy>
        {
            template <class State>
            static void execute(State const&,chain_row* tofill)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &Fsm::call_no_transition;
                tofill[state_id+1].one_state.push_back(reinterpret_cast<void*>(call_no_transition));
            }
        };
        template <class State>
        void operator()(State const& s)
        {
            helper<has_state_delayed_event<State,EventType>::type::value,
                   is_composite_state<State>::type::value>::execute(s,tofill_entries);
        }
        dispatch_table* self;
        chain_row* tofill_entries;
    };

    // variant for anonymous transitions
    template <class EventType>
    struct default_init_cell<EventType,
                             typename ::boost::enable_if<
                                typename is_completion_event<EventType>::type>::type>
    {
        default_init_cell(dispatch_table* self_,chain_row* tofill_entries_)
            : self(self_),tofill_entries(tofill_entries_)
        {}

        // this event is a compound one (not a real one, just one for use in event-less transitions)
        // Note this event cannot be used as deferred!
        template <class State>
        void operator()(State const&)
        {
            typedef typename create_stt<Fsm>::type stt;
            BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
            cell call_no_transition = &Fsm::default_eventless_transition;
            tofill_entries[state_id+1].one_state.push_back(call_no_transition);
        }

        dispatch_table* self;
        chain_row* tofill_entries;
    };

    template <typename T>
    using event_filter_predicate = is_base_of<typename T::transition_event, Event>;

 public:
    // initialize the dispatch table for a given Event and Fsm
    dispatch_table()
    {
        // Initialize cells for no transition
        mp11::mp_for_each<mp11::mp_copy_if<
            typename to_mp_list<Stt>::type,
            event_filter_predicate
            >>
            (init_cell(entries));

        mp11::mp_for_each<typename generate_state_set<Stt>::state_set_mp11>(
            default_init_cell<Event>{this, entries});
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
