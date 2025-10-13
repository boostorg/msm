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

#ifndef BOOST_MSM_BACKMP11_STATE_MACHINE_H
#define BOOST_MSM_BACKMP11_STATE_MACHINE_H

#include <algorithm>
#include <array>
#include <exception>
#include <functional>
#include <utility>

#include <boost/core/no_exceptions_support.hpp>
#include <boost/core/ignore_unused.hpp>

#include <boost/mp11.hpp>
#include <boost/mp11/mpl_list.hpp>

#include <boost/mpl/deref.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/for_each.hpp>

#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_convertible.hpp>

#include <boost/msm/active_state_switching_policies.hpp>
#include <boost/msm/row_tags.hpp>
#include <boost/msm/backmp11/metafunctions.hpp>
#include <boost/msm/backmp11/history_impl.hpp>
#include <boost/msm/backmp11/common_types.hpp>
#include <boost/msm/backmp11/dispatch_table.hpp>
#include <boost/msm/backmp11/favor_compile_time.hpp>
#include <boost/msm/backmp11/state_machine_config.hpp>

namespace boost { namespace msm { namespace backmp11
{

// event used internally for wrapping a direct entry
template <class StateType,class Event>
struct direct_entry_event
{
    typedef int direct_entry;
    typedef StateType active_state;
    typedef Event contained_event;

    direct_entry_event(Event const& evt):m_event(evt){}
    Event const& m_event;
};

// Back-end for state machines.
// Pass the state_machine_def as TFrontEnd,
// a state_machine_config as TConfig (optional, for customization),
// and the derived class as TDerived (required when extending the state_machine class).
template <
    class TFrontEnd,
    class TConfig = default_state_machine_config,
    class TDerived = void
>
class state_machine : public TFrontEnd
{
public:
    using Config = TConfig;
    using RootSm = typename Config::root_sm;
    using Context = typename Config::context;
    using FrontEnd = TFrontEnd;
    using Derived = mp11::mp_if<
        std::is_void<TDerived>,
        state_machine,
        TDerived
    >;

private:
    using CompilePolicy = typename Config::compile_policy;

    typedef ::std::function<
        HandledEnum ()>                          transition_fct;
    typedef ::std::function<
        HandledEnum () >                         deferred_fct;
    struct deferred_event_t
    {
        deferred_fct function;
        size_t seq_cnt;
    };
    using deferred_events_queue_t =
        typename Config::template queue_container<
            deferred_event_t>;
    using events_queue_t =
        typename Config::template queue_container<transition_fct>;

    typedef typename boost::mpl::eval_if<
        typename is_active_state_switch_policy<FrontEnd>::type,
        get_active_state_switch_policy<FrontEnd>,
        // default
        ::boost::mpl::identity<active_state_switch_after_entry>
    >::type active_state_switching;

    typedef bool (*flag_handler)(state_machine const&);

    // all state machines are friend with each other to allow embedding any of them in another fsm
    template <class, class, class>
    friend class state_machine;

    // Allow access to private members for serialization.
    template<typename T, typename A0, typename A1, typename A2>
    friend void serialize(T&, state_machine<A0, A1, A2>&);

    static constexpr int nr_regions = get_number_of_regions<typename FrontEnd::initial_state>::type::value;
    using active_state_ids_t = std::array<int, nr_regions>;

 public:
    // tags
    typedef int composite_tag;

    struct InitEvent { };
    struct ExitEvent { };
    // flag handling
    struct Flag_AND
    {
        typedef std::logical_and<bool> type;
    };
    struct Flag_OR
    {
     typedef std::logical_or<bool> type;
    };
    typedef typename FrontEnd::BaseAllStates  BaseState;

    // if the front-end fsm provides an initial_event typedef, replace InitEvent by this one
    typedef typename ::boost::mpl::eval_if<
        typename has_initial_event<FrontEnd>::type,
        get_initial_event<FrontEnd>,
        ::boost::mpl::identity<InitEvent>
    >::type fsm_initial_event;

    // if the front-end fsm provides an exit_event typedef, replace ExitEvent by this one
    typedef typename ::boost::mpl::eval_if<
        typename has_final_event<FrontEnd>::type,
        get_final_event<FrontEnd>,
        ::boost::mpl::identity<ExitEvent>
    >::type fsm_final_event;

    template <class ExitPoint>
    struct exit_pt : public ExitPoint
    {
        // tags
        typedef ExitPoint           wrapped_exit;
        typedef int                 pseudo_exit;
        typedef Derived             owner;
        typedef int                 no_automatic_create;
        typedef typename
            ExitPoint::event        Event;
        typedef std::function<HandledEnum (Event const&)>
                                    forward_function;

        // forward event to the higher-level FSM
        template <class ForwardEvent>
        void forward_event(ForwardEvent const& incomingEvent)
        {
            // use helper to forward or not
            ForwardHelper< ::boost::is_convertible<ForwardEvent,Event>::value>::helper(incomingEvent,m_forward);
        }
        void set_forward_fct(forward_function fct)
        {
            m_forward = fct;
        }
        exit_pt():m_forward(){}
        // by assignments, we keep our forwarding functor unchanged as our containing SM did not change
    template <class RHS>
        exit_pt(RHS&):m_forward(){}
        exit_pt<ExitPoint>& operator= (const exit_pt<ExitPoint>& )
        {
            return *this;
        }
    private:
         forward_function          m_forward;

         // using partial specialization instead of enable_if because of VC8 bug
        template <bool OwnEvent, int Dummy=0>
        struct ForwardHelper
        {
            template <class ForwardEvent>
            static void helper(ForwardEvent const& ,forward_function& )
            {
                // Not our event, assert
                BOOST_ASSERT(false);
            }
        };
        template <int Dummy>
        struct ForwardHelper<true,Dummy>
        {
            template <class ForwardEvent>
            static void helper(ForwardEvent const& incomingEvent,forward_function& forward_fct)
            {
                // call if handler set, if not, this state is simply a terminate state
                if (forward_fct)
                    forward_fct(incomingEvent);
            }
        };

    };
    template <class EntryPoint>
    struct entry_pt : public EntryPoint
    {
        // tags
        typedef EntryPoint          wrapped_entry;
        typedef int                 pseudo_entry;
        typedef Derived             owner;
        typedef int                 no_automatic_create;
    };
    template <class EntryPoint>
    struct direct : public EntryPoint
    {
        // tags
        typedef EntryPoint          wrapped_entry;
        typedef int                 explicit_entry_state;
        typedef Derived             owner;
        typedef int                 no_automatic_create;
    };

    // Template used to create transitions from rows in the transition table
    // (normal transitions).
    template<typename Row, bool HasAction, bool HasGuard>
    struct Transition
    {
        //typedef typename ROW::Source T1;
        typedef typename make_entry<typename Row::Source,state_machine>::type T1;
        typedef typename make_exit<typename Row::Target,state_machine>::type T2;
        typedef typename Row::Evt transition_event;
        // if the source is an exit pseudo state, then
        // current_state_type becomes the result of get_owner
        // meaning the containing SM from which the exit occurs
        typedef typename ::boost::mpl::eval_if<
                typename has_pseudo_exit<T1>::type,
                get_owner<T1,Derived>,
                ::boost::mpl::identity<typename Row::Source> >::type current_state_type;

        // if Target is a sequence, then we have a fork and expect a sequence of explicit_entry
        // else if Target is an explicit_entry, next_state_type becomes the result of get_owner
        // meaning the containing SM if the row is "outside" the containing SM or else the explicit_entry state itself
        typedef typename ::boost::mpl::eval_if<
            typename ::boost::mpl::is_sequence<T2>::type,
            get_fork_owner<T2,Derived>,
            ::boost::mpl::eval_if<
                    typename has_no_automatic_create<T2>::type,
                    get_owner<T2,Derived>,
                    ::boost::mpl::identity<T2> >
        >::type next_state_type;

        // if a guard condition is here, call it to check that the event is accepted
        static bool check_guard(state_machine& fsm,transition_event const& evt)
        {
            if constexpr (HasGuard)
            {
                return Row::guard_call(fsm,evt,
                                 fsm.get_state<current_state_type>(),
                                 fsm.get_state<next_state_type>(),
                                 fsm.m_states);
            }
            else
            {
                return true;
            }
        }
        // Take the transition action and return the next state.
        static HandledEnum execute(state_machine& fsm, int region_index, int state, transition_event const& evt)
        {
            BOOST_STATIC_CONSTANT(int, current_state = (get_state_id<current_state_type>()));
            BOOST_STATIC_CONSTANT(int, next_state = (get_state_id<next_state_type>()));
            boost::ignore_unused(state); // Avoid warnings if BOOST_ASSERT expands to nothing.
            BOOST_ASSERT(state == (current_state));
            // if T1 is an exit pseudo state, then take the transition only if the pseudo exit state is active
            if (has_pseudo_exit<T1>::type::value &&
                !backmp11::is_exit_state_active<T1,get_owner<T1,Derived> >(fsm))
            {
                return HANDLED_FALSE;
            }
            if (!check_guard(fsm,evt))
            {
                // guard rejected the event, we stay in the current one
                return HANDLED_GUARD_REJECT;
            }

            fsm.m_active_state_ids[region_index] = active_state_switching::after_guard(current_state,next_state);

            // first call the exit method of the current state
            execute_exit<current_state_type>(fsm.get_state<current_state_type>(),evt,fsm);
            fsm.m_active_state_ids[region_index] = active_state_switching::after_exit(current_state,next_state);

            // then call the action method
            HandledEnum res;
            if constexpr (HasAction)
            {
                res = Row::action_call(fsm,evt,
                                fsm.get_state<current_state_type>(),
                                fsm.get_state<next_state_type>(),
                                fsm.m_states);
            }
            else
            {
                res = HANDLED_TRUE;
            }

            fsm.m_active_state_ids[region_index] = active_state_switching::after_action(current_state,next_state);

            // and finally the entry method of the new current state
            convert_event_and_execute_entry<next_state_type,T2>
                (fsm.get_state<next_state_type>(),evt,fsm);
            fsm.m_active_state_ids[region_index] = active_state_switching::after_entry(current_state,next_state);

            return res;
        }
    };

    // Template used to create transitions from rows in the transition table
    // (internal transitions).
    template<typename Row, bool HasAction, bool HasGuard>
    struct ITransition
    {
        typedef typename Row::Evt transition_event;
        typedef typename Row::Source current_state_type;
        typedef typename make_exit<typename Row::Target,state_machine>::type next_state_type;

        // if a guard condition is here, call it to check that the event is accepted
        static bool check_guard(state_machine& fsm,transition_event const& evt)
        {
            if constexpr (HasGuard)
            {
                return Row::guard_call(fsm,evt,
                                 fsm.get_state<current_state_type>(),
                                 fsm.get_state<next_state_type>(),
                                 fsm.m_states);
            }
            else
            {
                return true;
            }
        }
        // Take the transition action and return the next state.
        static HandledEnum execute(state_machine& fsm, int , int state, transition_event const& evt)
        {

            BOOST_STATIC_CONSTANT(int, current_state = (get_state_id<current_state_type>()));
            boost::ignore_unused(state, current_state); // Avoid warnings if BOOST_ASSERT expands to nothing.
            BOOST_ASSERT(state == (current_state));
            if (!check_guard(fsm,evt))
            {
                // guard rejected the event, we stay in the current one
                return HANDLED_GUARD_REJECT;
            }

            // call the action method
            if constexpr (HasAction)
            {
                return Row::action_call(fsm,evt,
                             fsm.get_state<current_state_type>(),
                             fsm.get_state<next_state_type>(),
                             fsm.m_states);
            }
            else
            {
                return HANDLED_TRUE;
            }
        }
    };

    // Template used to create transitions from rows in the transition table
    // (SM-internal transitions).
    template<typename Row, bool HasAction, bool HasGuard, typename StateType>
    struct InternalTransition
    {
        typedef StateType current_state_type;
        typedef StateType next_state_type;
        typedef typename Row::Evt transition_event;

        // if a guard condition is here, call it to check that the event is accepted
        static bool check_guard(state_machine& fsm,transition_event const& evt)
        {
            if constexpr (HasGuard)
            {
                if constexpr (std::is_same_v<StateType, state_machine>)
                {
                    return Row::guard_call(fsm,evt,
                        fsm,
                        fsm,
                        fsm.m_states);                    
                }
                else
                {
                    return Row::guard_call(fsm,evt,
                        fsm.get_state<StateType>(),
                        fsm.get_state<StateType>(),
                        fsm.m_states);
                }
            }
            else
            {
                return true;
            }
        }
        // Take the transition action and return the next state.
        static HandledEnum execute(state_machine& fsm, int , int , transition_event const& evt)
        {
            if (!check_guard(fsm,evt))
            {
                // guard rejected the event, we stay in the current one
                return HANDLED_GUARD_REJECT;
            }

            // then call the action method
            if constexpr (HasAction)
            {
                if constexpr(std::is_same_v<StateType, state_machine>)
                {
                    return Row::action_call(fsm,evt,
                        fsm,
                        fsm,
                        fsm.m_states);
                }
                else
                {
                    return Row::action_call(fsm,evt,
                        fsm.get_state<StateType>(),
                        fsm.get_state<StateType>(),
                        fsm.m_states);
                }
            }
            else
            {
                return HANDLED_TRUE;
            }
        }
    };

    // Template used to form forwarding rows in the transition table for every row of a composite SM
    template<
        typename T1
        , class Evt
    >
    struct frow
    {
        typedef T1                  current_state_type;
        typedef T1                  next_state_type;
        typedef Evt                 transition_event;
        // tag to find out if a row is a forwarding row
        typedef int                 is_frow;

        // Take the transition action and return the next state.
        static HandledEnum execute(state_machine& fsm, int region_index, int , transition_event const& evt)
        {
            // false as second parameter because this event is forwarded from outer fsm
            HandledEnum res =
                (fsm.get_state<current_state_type>()).process_event_internal(evt);
            fsm.m_active_state_ids[region_index]=get_state_id<T1>();
            return res;
        }
        // helper metafunctions used by dispatch table and give the frow a new event
        // (used to avoid double entries in a table because of base events)
        template <class NewEvent>
        struct replace_event
        {
            typedef frow<T1,NewEvent> type;
        };
    };

    template <class Tag, class Row,class StateType>
    struct create_backend_stt;
    template <class Row,class StateType>
    struct create_backend_stt<g_row_tag,Row,StateType>
    {
        using type = Transition<Row, false, true>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<a_row_tag,Row,StateType>
    {
        using type = Transition<Row, true, false>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<_row_tag,Row,StateType>
    {
        using type = Transition<Row, false, false>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<row_tag,Row,StateType>
    {
        using type = Transition<Row, true, true>;
    };
    // internal transitions
    template <class Row,class StateType>
    struct create_backend_stt<g_irow_tag,Row,StateType>
    {
        using type = ITransition<Row, false, true>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<a_irow_tag,Row,StateType>
    {
        using type = ITransition<Row, true, false>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<irow_tag,Row,StateType>
    {
        using type = ITransition<Row, true, true>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<_irow_tag,Row,StateType>
    {
        using type = ITransition<Row, false, false>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<sm_a_i_row_tag,Row,StateType>
    {
        using type = InternalTransition<Row, true, false, StateType>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<sm_g_i_row_tag,Row,StateType>
    {
        using type = InternalTransition<Row, false, true, StateType>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<sm_i_row_tag,Row,StateType>
    {
        using type = InternalTransition<Row, true, true, StateType>;
    };
    template <class Row,class StateType>
    struct create_backend_stt<sm__i_row_tag,Row,StateType>
    {
        using type = InternalTransition<Row, false, false, StateType>;
    };
    template <class Row,class StateType=void>
    struct make_row_tag
    {
        using type = typename create_backend_stt<typename Row::row_type_tag,Row,StateType>::type;
    };

    // add to the stt the initial states which could be missing (if not being involved in a transition)
    template <class BaseType, class stt_simulated = typename BaseType::transition_table>
    struct create_real_stt
    {
        //typedef typename BaseType::transition_table stt_simulated;
        template<typename T>
        using make_row_tag_base_type = typename make_row_tag<T, BaseType>::type;
        typedef typename boost::mp11::mp_transform<
            make_row_tag_base_type,
            typename to_mp_list<stt_simulated>::type
        > type;
    };

    template <class Table,class Intermediate,class StateType>
    struct add_forwarding_row_helper
    {
        typedef typename generate_event_set<Table>::event_set_mp11 all_events;

        template<typename T>
        using frow_state_type = frow<StateType, T>;
        typedef mp11::mp_append<
            typename to_mp_list<Intermediate>::type,
            mp11::mp_transform<frow_state_type, all_events>
            > type;
    };
    // gets the transition table from a composite and make from it a forwarding row
    template <class StateType,class IsComposite>
    struct get_internal_transition_table
    {
        // first get the table of a composite
        typedef typename recursive_get_transition_table<StateType>::type original_table;

        // we now look for the events the composite has in its internal transitions
        // the internal ones are searched recursively in sub-sub... states
        // we go recursively because our states can also have internal tables or substates etc.
        typedef typename recursive_get_internal_transition_table<StateType, ::boost::mpl::true_>::type recursive_istt;
        template<typename T>
        using make_row_tag_state_type = typename make_row_tag<T, StateType>::type;
        typedef boost::mp11::mp_transform<
            make_row_tag_state_type,
            typename to_mp_list<recursive_istt>::type
        > recursive_istt_with_tag;

        typedef boost::mp11::mp_append<original_table, recursive_istt_with_tag> table_with_all_events;

        // and add for every event a forwarding row
        typedef typename ::boost::mpl::eval_if<
                typename CompilePolicy::add_forwarding_rows,
                add_forwarding_row_helper<table_with_all_events,mp11::mp_list<>,StateType>,
                ::boost::mpl::identity< mp11::mp_list<> >
        >::type type;
    };
    template <class StateType>
    struct get_internal_transition_table<StateType, ::boost::mpl::false_ >
    {
        typedef typename create_real_stt<StateType, typename StateType::internal_transition_table >::type type;
    };
    // typedefs used internally
    typedef typename create_real_stt<FrontEnd>::type real_transition_table;
    typedef typename create_stt<state_machine>::type stt;
    typedef typename get_initial_states<typename FrontEnd::initial_state>::type initial_states;
    typedef typename generate_state_set<stt>::state_set_mp11 state_set_mp11;
    typedef typename generate_state_map<stt>::type state_map_mp11;
    typedef typename generate_event_set<stt>::event_set_mp11 event_set_mp11;
    typedef history_impl<typename FrontEnd::history, nr_regions> concrete_history;
    typedef mp11::mp_rename<state_set_mp11, std::tuple> substate_list;
    typedef typename generate_event_set<
        typename create_real_stt<state_machine, typename state_machine::internal_transition_table >::type
    >::event_set_mp11 processable_events_internal_table;

    // extends the transition table with rows from composite states
    template <class Composite>
    struct extend_table
    {
        // add the init states
        //typedef typename create_stt<Composite>::type stt;
        typedef typename Composite::stt Stt;

        // add the internal events defined in the internal_transition_table
        // Note: these are added first because they must have a lesser prio
        // than the deeper transitions in the sub regions
        // table made of a stt + internal transitions of composite
        template<typename T>
        using make_row_tag_composite = typename make_row_tag<T, Composite>::type;
        typedef typename boost::mp11::mp_transform<
            make_row_tag_composite,
            typename to_mp_list<typename Composite::internal_transition_table>::type
        > internal_stt;

        typedef boost::mp11::mp_append<
            typename to_mp_list<Stt>::type,
            internal_stt
        > stt_plus_internal;

        // for every state, add its transition table (if any)
        // transformed as frow
        template<typename V, typename T>
        using F = boost::mp11::mp_append<
            V,
            typename get_internal_transition_table<T, typename is_composite_state<T>::type>::type
            >;
        typedef boost::mp11::mp_fold<
            state_set_mp11,
            stt_plus_internal,
            F
        > type;
    };
    // extend the table with tables from composite states
    typedef typename extend_table<state_machine>::type complete_table;
    // define the dispatch table used for event dispatch
    typedef dispatch_table<state_machine,CompilePolicy> sm_dispatch_table;
    // build a sequence of regions
    typedef typename get_regions_as_sequence<typename FrontEnd::initial_state>::type seq_initial_states;

    // For some reason these declarations have to appear after the instantiation of real_transition_table.
  private:
    struct deferred_events_t
    {
        deferred_events_queue_t queue;
        size_t cur_seq_cnt;
    };
    using deferred_events_member =
        optional_instance<
            deferred_events_t,
            has_fsm_deferred_events<state_machine>::value
        >;
    using events_queue_member = 
        optional_instance<
            events_queue_t,
            !is_no_message_queue<state_machine>::value
        >;
    using context_member =
        optional_instance<
            Context*,
            !std::is_same_v<Context, no_context> &&
            (std::is_same_v<RootSm, no_root_sm> || std::is_same_v<RootSm, Derived>)>;

  public:
    // Construct with the default initial states and forward constructor arguments to the front-end.
    template <typename... Args>
    state_machine(Args&&... args)
        : FrontEnd(std::forward<Args>(args)...)
    {
        if constexpr (!std::is_same_v<Derived, state_machine>) {
            static_assert(
                std::is_constructible_v<Derived, Args...>,
                "Derived class must inherit state_machine's constructors."
            );
        }
        // initialize our list of states with the ones defined in FrontEnd::initial_state
        ::boost::mpl::for_each< seq_initial_states, ::boost::msm::wrap<mpl::placeholders::_1> >
                       (set_initial_states_helper(m_active_state_ids));
        m_history.set_initial_states(m_active_state_ids);
        if constexpr (std::is_same_v<RootSm, no_root_sm> ||
                      std::is_same_v<RootSm, Derived>)
        {
            // create states
            init(*static_cast<Derived*>(this));
        }
    }

    template <bool C = context_member::value,
              typename = std::enable_if_t<C>,
              typename... Args>
    state_machine(Context& context, Args&&... args)
        : state_machine(std::forward<Args>(args)...)
        {
            m_optional_members.template get<context_member>() = &context;
        }
    

    // assignment operator using the copy policy to decide if non_copyable, shallow or deep copying is necessary
    state_machine& operator= (state_machine const& rhs)
    {
        if (this != &rhs)
        {
           FrontEnd::operator=(rhs);
           do_copy(rhs);
        }
       return *this;
    }
    state_machine(state_machine const& rhs)
        : FrontEnd(rhs)
    {
       if (this != &rhs)
       {
           // initialize our list of states with the ones defined in FrontEnd::initial_state
           init(*this);
           do_copy(rhs);
       }
    }

    // start the state machine (calls entry of the initial state)
    void start()
    {
        start(fsm_initial_event{});
    }

    // start the state machine (calls entry of the initial state passing incomingEvent to on_entry's)
    template <class Event>
    void start(Event const& incomingEvent)
    {
        m_running = true;
        // reinitialize our list of currently active states with the ones defined in FrontEnd::initial_state
        ::boost::mpl::for_each< seq_initial_states, ::boost::msm::wrap<mpl::placeholders::_1> >
                        (set_initial_states_helper(m_active_state_ids));
        // call on_entry on this SM and all initial states
        static_cast<FrontEnd*>(this)->on_entry(incomingEvent,*static_cast<Derived*>(this));
        using initial_states_mp11 = to_mp_list_t<initial_states>;
        mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, initial_states_mp11>>(
            [this, &incomingEvent](auto state_identity)
            {
                using State = typename decltype(state_identity)::type;
                execute_entry(this->get_state<State>(), incomingEvent, *this);
            }
        );
        // give a chance to handle an anonymous (eventless) transition
        try_process_completion_event(EVENT_SOURCE_DEFAULT, true);
    }

    // stop the state machine (calls exit of the current state)
    void stop()
    {
        stop(fsm_final_event{});
    }

    // stop the state machine (calls exit of the current state passing finalEvent to on_exit's)
    template <class Event>
    void stop(Event const& finalEvent)
    {
        do_exit(finalEvent,*this);
        m_running = false;
    }

    // Main function used by clients of the derived FSM to make transitions.
    template<class Event>
    HandledEnum process_event(Event const& evt)
    {
        return process_event_internal(evt, EVENT_SOURCE_DIRECT);
    }

    // Enqueues an event in the message queue.
    // Call process_queued_events to process all queued events.
    // Be careful if you do this during event processing, the event will be processed immediately
    // and not kept in the queue.
    template <class EventType,
              bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    void enqueue_event(EventType const& evt)
    {
        get_events_queue().push_back(
            [this, evt]
            {
                return process_event_internal(evt, static_cast<EventSource>(EVENT_SOURCE_MSG_QUEUE));
            }
        );
    }

    // Process all queued events.
    template <bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    void process_queued_events()
    {
        while(!get_events_queue().empty())
        {
            process_single_queued_event();
        }
    }

    // Process a single queued event.
    template <bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    void process_single_queued_event()
    {
        transition_fct to_call = get_events_queue().front();
        get_events_queue().pop_front();
        to_call();
    }

    template <bool C = !std::is_same_v<Context, no_context>,
              typename = std::enable_if_t<C>>
    Context& get_context()
    {
        if constexpr (context_member::value)
        {
            return *m_optional_members.template get<context_member>();
        }
        else
        {
            return get_root_sm().get_context();
        }
    }

    template <bool C = !std::is_same_v<Context, no_context>,
              typename = std::enable_if_t<C>>
    const Context& get_context() const
    {
        if constexpr (context_member::value)
        {
            return *m_optional_members.template get<context_member>();
        }
        else
        {
            return get_root_sm().get_context();
        }
    }

    template <bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    events_queue_t& get_events_queue()
    {
        return m_optional_members.template get<events_queue_member>();
    }

    template <bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    const events_queue_t& get_events_queue() const
    {
       return m_optional_members.template get<events_queue_member>();
    }

  private:
    template <bool C = deferred_events_member::value,
              typename = std::enable_if_t<C>>
    deferred_events_t& get_deferred_events()
    {
        return m_optional_members.template get<deferred_events_member>();
    }

    template <bool C = deferred_events_member::value,
              typename = std::enable_if_t<C>>
    const deferred_events_t& get_deferred_events() const
    {
        return m_optional_members.template get<deferred_events_member>();
    }

  public:
    template <bool C = deferred_events_member::value,
              typename = std::enable_if_t<C>>
    deferred_events_queue_t& get_deferred_events_queue()
    {
        return get_deferred_events().queue;
    }

    template <bool C = deferred_events_member::value,
              typename = std::enable_if_t<C>>
    const deferred_events_queue_t& get_deferred_events_queue() const
    {
        return get_deferred_events().queue;
    }

    // Getter that returns the currently active state IDS of the FSM.
    const active_state_ids_t& get_active_state_ids() const
    {
        return m_active_state_ids;
    }

    // Get the root sm.
    template <typename T = RootSm, 
              typename = std::enable_if_t<!std::is_same_v<T, no_root_sm>>>
    RootSm& get_root_sm()
    {
        return *static_cast<RootSm*>(m_root_sm);
    }
    // Get the root sm.
    template <typename T = RootSm, 
              typename = std::enable_if_t<!std::is_same_v<T, no_root_sm>>>
    const RootSm& get_root_sm() const
    {
        return *static_cast<const RootSm*>(m_root_sm);
    }

    // Return the id of a state in the sm.
    template<typename State>
    static constexpr int get_state_id(const State&)
    {
        return backmp11::get_state_id<stt,State>::type::value;
    }
    // Return the id of a state in the sm.
    template<typename State>
    static constexpr int get_state_id()
    {
        return backmp11::get_state_id<stt,State>::type::value;
    }

    // true if the sm is used in another sm
    bool is_contained() const
    {
        return (static_cast<const void*>(this) != m_root_sm);
    }

    // Get a state.
    template <class State>
    State& get_state()
    {
        return std::get<std::remove_reference_t<State>>(m_states);
    }
    // Get a state (const version).
    template <class State>
    const State& get_state() const
    {
        return std::get<std::remove_reference_t<State>>(m_states);
    }

    // checks if a flag is active using the BinaryOp as folding function
    template <class Flag,class BinaryOp>
    bool is_flag_active() const
    {
        flag_handler* flags_entries = get_entries_for_flag<Flag>();
        bool res = (*flags_entries[ m_active_state_ids[0] ])(*this);
        for (int i = 1; i < nr_regions ; ++i)
        {
            res = typename BinaryOp::type() (res,(*flags_entries[ m_active_state_ids[i] ])(*this));
        }
        return res;
    }
    // checks if a flag is active using no binary op if 1 region, or OR if > 1 regions
    template <class Flag>
    bool is_flag_active() const
    {
        return FlagHelper<Flag,(nr_regions>1)>::helper(*this,get_entries_for_flag<Flag>());
    }

    // Visit the states (only active states).
    template <typename Visitor>
    void visit(Visitor&& visitor)
    {
        visit(std::forward<Visitor>(visitor), active_states);
    }

    // Visit the states (only active states).
    template <typename Visitor>
    void visit(Visitor&& visitor, active_states_t)
    {
        if (m_running)
        {
            for (int i = 0; i < nr_regions; ++i)
            {
                visitor_dispatch_table<Visitor>::dispatch(*this, m_active_state_ids[i], std::forward<Visitor>(visitor));
            }
        }
    }

    // Visit the states (all states).
    template <typename Visitor>
    void visit(Visitor&& visitor, all_states_t)
    {
        mp11::tuple_for_each(m_states,
            [&visitor](auto& state)
            {
                std::invoke(std::forward<Visitor>(visitor), state);

                using State = std::remove_reference_t<decltype(state)>;
                if constexpr (is_composite_state<State>::value)
                {
                    state.visit(std::forward<Visitor>(visitor), all_states);
                }
            }
        );
    }

  private:
    template <class Event>
    void do_defer_event(const Event& event)
    {
        // Deferred events are added with a correlation sequence that helps to
        // identify when an event was added - This is typically to distinguish
        // between events deferred in this processing versus previous.
        deferred_events_t& deferred_events = get_deferred_events();
        deferred_events.queue.push_back(
            deferred_event_t
            {
                [this, event]
                {
                    return process_event_internal(
                        event,
                        static_cast<EventSource>(EVENT_SOURCE_DIRECT|EVENT_SOURCE_DEFERRED));
                },
                deferred_events.cur_seq_cnt + 1
            }
        );
    }

public:
    // puts the given event into the deferred queue
    template <
        class Event,
        bool C = deferred_events_member::value,
        typename = std::enable_if_t<C>>
    void defer_event(Event const& event)
    {
        if constexpr (
            is_kleene_event<Event>::value &&
            std::is_same_v<CompilePolicy, favor_runtime_speed>)
        {
            typedef typename generate_event_set<stt>::event_set_mp11 event_list;
            bool found =
                for_each_until<mp11::mp_transform<mp11::mp_identity, event_list>>(
                    [this, &event](auto event_identity)
                    {
                        using KnownEvent = typename decltype(event_identity)::type;
                        if (event.type() == typeid(KnownEvent))
                        {
                            this->do_defer_event(*any_cast<KnownEvent>(&event));
                            return true;
                        }
                        return false;
                    }
            );
            if (!found)
            {
                for (const auto nr_region : get_active_state_ids())
                {
                    this->no_transition(event, *this, nr_region);
                }
            }
        }
        else
        {
            do_defer_event(event);
        }
    }

 protected:    // interface for the derived class

    // Checks if an event is an end interrupt event.
    template <class Event, class Policy = CompilePolicy>
    typename std::enable_if<std::is_same<Policy, favor_runtime_speed>::value, bool>::type
    is_end_interrupt_event(const Event&)
    {
        return is_flag_active<EndInterruptFlag<Event>>();
    }
    template <class Policy = CompilePolicy>
    typename std::enable_if<std::is_same<Policy, favor_compile_time>::value, bool>::type
    is_end_interrupt_event(const any_event& event)
    {
        static end_interrupt_event_helper helper{*this};
        return helper.is_end_interrupt_event(event);
    }

     // helper used to fill the initial states
     struct set_initial_states_helper
     {
         set_initial_states_helper(active_state_ids_t& init):m_initial_states(init),m_index(-1){}

         // History initializer function object, used with mpl::for_each
         template <class State>
         void operator()(::boost::msm::wrap<State> const&)
         {
             m_initial_states[++m_index]=get_state_id<State>();
         }
         active_state_ids_t& m_initial_states;
         int m_index;
     };
    
    // handling of deferred events
    void try_process_deferred_events(bool new_seq = false)
    {
        if constexpr (deferred_events_member::value)
        {
            deferred_events_t& deferred_events = get_deferred_events();
            // A new sequence is typically started upon initial entry to the
            // state, or upon a new transition.  When this occurs we want to
            // process all previously deferred events by incrementing the
            // correlation sequence.
            if (new_seq)
            {
                deferred_events.cur_seq_cnt += 1;
            }

            // Iteratively process all of the events within the deferred
            // queue upto (but not including) newly deferred events.
            // if we did not defer one in the queue, then we need to try again
            bool not_only_deferred = false;
            while (!deferred_events.queue.empty())
            {
                deferred_event_t& deferred_event =
                    deferred_events.queue.front();

                if (deferred_events.cur_seq_cnt != deferred_event.seq_cnt)
                {
                    break;
                }

                deferred_fct next = deferred_event.function;
                deferred_events.queue.pop_front();
                HandledEnum res = next();
                if (res != HANDLED_FALSE && res != HANDLED_DEFERRED)
                {
                    not_only_deferred = true;
                }
                if (not_only_deferred)
                {
                    // handled one, stop processing deferred until next block reorders
                    break;
                }
            }
            if (not_only_deferred)
            {
                // attempt to go back to the situation prior to processing, 
                // in case some deferred events would have been re-queued
                // in that case those would have a higher sequence number
                std::stable_sort(
                    deferred_events.queue.begin(),
                    deferred_events.queue.end(),
                    [](const deferred_event_t& a, const deferred_event_t& b)
                    {
                        return a.seq_cnt > b.seq_cnt;
                    }
                );
                // reset sequence number for all
                std::for_each(
                    deferred_events.queue.begin(),
                    deferred_events.queue.end(),
                    [&deferred_events](deferred_event_t& deferred_event)
                    {
                        deferred_event.seq_cnt = deferred_events.cur_seq_cnt + 1;
                    }
                );
                // one deferred event was successfully processed, try again
                try_process_deferred_events(true);
            }
        }
    }

    // handling of eventless transitions
    void try_process_completion_event(EventSource source, bool handled)
    {
        // if none is found in the SM, nothing to do
        if constexpr (has_fsm_eventless_transition<state_machine>::value)
        {
            typedef typename ::boost::mpl::deref<
                typename ::boost::mpl::begin<
                    typename find_completion_events<state_machine>::type
                        >::type
            >::type first_completion_event;
            if (handled)
            {
                process_event_internal(
                    first_completion_event(),
                    source | EVENT_SOURCE_DIRECT);
            }
        }
    }

    // Handling of enqueued events.
    void try_process_queued_events()
    {
        if constexpr (events_queue_member::value)
        {
            process_queued_events();
        }
    }

    // Main function used internally to make transitions
    // Can only be called for internally (for example in an action method) generated events.
    template<class Event>
    HandledEnum process_event_internal(Event const& evt,
                           EventSource source = EVENT_SOURCE_DEFAULT)
    {
        if constexpr (std::is_same_v<CompilePolicy, favor_compile_time>)
        {
            if constexpr (std::is_same_v<Event, any_event>)
            {
                return process_event_internal_impl(evt, source);
            }
            else
            {
                return process_event_internal_impl(any_event(evt), source);
            }
        }
        else
        {
            return process_event_internal_impl(evt, source);
        }
    }
    template<class Event>
    HandledEnum process_event_internal_impl(Event const& evt, EventSource source)
    {
        // If the state machine has terminate or interrupt flags, check them.
        if constexpr (has_fsm_blocking_states<state_machine>::value)
        {
            // If the state machine is terminated, do not handle any event.
            if (is_flag_active<TerminateFlag>())
            {
                return HANDLED_TRUE;
            }
            // If the state machine is interrupted, do not handle any event
            // unless the event is the end interrupt event.
            if (is_flag_active<InterruptedFlag>() && !is_end_interrupt_event(evt))
            {
                return HANDLED_TRUE;
            }
        }

        // If we have an event queue and are already processing events,
        // enqueue it for later processing.
        if constexpr (events_queue_member::value)
        {
            if (m_event_processing)
            {
                get_events_queue().push_back(
                    [this, evt]
                    {
                        return process_event_internal(
                            evt,
                            static_cast<EventSource>(EVENT_SOURCE_DIRECT | EVENT_SOURCE_MSG_QUEUE));
                    }
                );
                return HANDLED_TRUE;
            }
        }

        // Process the event.
        m_event_processing = true;
        HandledEnum handled;
        const bool is_direct_call = source & EVENT_SOURCE_DIRECT;
        if constexpr (is_no_exception_thrown<state_machine>::value)
        {
            handled = do_process_event(evt, is_direct_call);
        }
        else
        {
            // when compiling without exception support there is no formal parameter "e" in the catch handler.
            // Declaring a local variable here does not hurt and will be "used" to make the code in the handler
            // compilable although the code will never be executed.
            std::exception e;
            BOOST_TRY
            {
                handled = do_process_event(evt, is_direct_call);
            }
            BOOST_CATCH (std::exception& e)
            {
                // give a chance to the concrete state machine to handle
                this->exception_caught(evt, *this, e);
                handled = HANDLED_FALSE;
            }
            BOOST_CATCH_END
        }

        // at this point we allow the next transition be executed without enqueing
        // so that completion events and deferred events execute now (if any)
        m_event_processing = false;

        // Process completion transitions BEFORE any other event in the
        // pool (UML Standard 2.3 15.3.14)
        try_process_completion_event(source, (handled & HANDLED_TRUE));

        // After handling, take care of the queued and deferred events.
        // Default:
        // Handle deferred events queue with higher prio than events queue.
        if constexpr (!has_event_queue_before_deferred_queue<state_machine>::value)
        {
            if (!(EVENT_SOURCE_DEFERRED & source))
            {
                try_process_deferred_events(HANDLED_TRUE & handled);

                // Handle any new events generated into the queue, but only if
                // we're not already processing from the message queue.
                if (!(EVENT_SOURCE_MSG_QUEUE & source))
                {
                    try_process_queued_events();
                }
            }
        }
        // Non-default:
        // Handle events queue with higher prio than deferred events queue.
        else
        {
            if (!(EVENT_SOURCE_MSG_QUEUE & source))
            {
                try_process_queued_events();
                if (!(EVENT_SOURCE_DEFERRED & source))
                {
                    try_process_deferred_events(HANDLED_TRUE & handled);
                }
            }
        }

        return handled;
    }

    // minimum event processing without exceptions, queues, etc.
    template<class Event>
    HandledEnum do_process_event(Event const& evt, bool is_direct_call)
    {
        HandledEnum handled = HANDLED_FALSE;
        // Dispatch the event to every region.
        for (int region_id=0; region_id<nr_regions; region_id++)
        {
            handled = static_cast<HandledEnum>(
                static_cast<int>(handled) |
                static_cast<int>(sm_dispatch_table::dispatch(*this, region_id, m_active_state_ids[region_id], evt))
            );
        }
        // Process the event in the internal table of this fsm if the event is processable (present in the table).
        if constexpr (mp11::mp_set_contains<processable_events_internal_table,Event>::value)
        {
            handled = static_cast<HandledEnum>(
                static_cast<int>(handled) |
                static_cast<int>(sm_dispatch_table::dispatch_internal(*this, 0, m_active_state_ids[0], evt))
            );
        }

        // if the event has not been handled and we have orthogonal zones, then
        // generate an error on every active state
        // for state machine states contained in other state machines, do not handle
        // but let the containing sm handle the error, unless the event was generated in this fsm
        // (by calling process_event on this fsm object, is_direct_call == true)
        // completion events do not produce an error
        if ( (!is_contained() || is_direct_call) && !handled && !is_completion_event<Event, CompilePolicy>::value(evt))
        {
            for (int i=0; i<nr_regions;++i)
            {
                this->no_transition(evt,*this,this->m_active_state_ids[i]);
            }
        }
        return handled;
    }

private:
    // helper for flag handling. Uses OR by default on orthogonal zones.
    template <class Flag,bool orthogonalStates>
    struct FlagHelper
    {
        static bool helper(state_machine const& sm,flag_handler* )
        {
            // by default we use OR to accumulate the flags
            return sm.is_flag_active<Flag,Flag_OR>();
        }
    };
    template <class Flag>
    struct FlagHelper<Flag,false>
    {
        static bool helper(state_machine const& sm,flag_handler* flags_entries)
        {
            // just one active state, so we can call operator[] with 0
            return flags_entries[sm.get_active_state_ids()[0]](sm);
        }
    };
    // handling of flag
    // defines a true and false functions plus a forwarding one for composite states
    template <class StateType,class Flag>
    struct FlagHandler
    {
        static bool flag_true(state_machine const& )
        {
            return true;
        }
        static bool flag_false(state_machine const& )
        {
            return false;
        }
        static bool forward(state_machine const& fsm)
        {
            return fsm.get_state<StateType>().template is_flag_active<Flag>();
        }
    };
    template <class Flag>
    struct init_flags
    {
    private:
        // helper function, helps hiding the forward function for non-state machines states.
        template <class T>
        void helper (flag_handler* an_entry,int offset, ::boost::mpl::true_ const &  )
        {
            // composite => forward
            an_entry[offset] = &FlagHandler<T,Flag>::forward;
        }
        template <class T>
        void helper (flag_handler* an_entry,int offset, ::boost::mpl::false_ const &  )
        {
            // default no flag
            an_entry[offset] = &FlagHandler<T,Flag>::flag_false;
        }
        // attributes
        flag_handler* entries;

    public:
        init_flags(flag_handler* entries_)
            : entries(entries_)
        {}

        // Flags initializer function object, used with for_each
        template <class StateType>
        void operator()( mp11::mp_identity<StateType> const& )
        {
            typedef typename get_flag_list<StateType>::type flags;
            typedef mp11::mp_contains<flags,Flag > found;

            BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<StateType>()));
            if (found::type::value)
            {
                // the type defined the flag => true
                entries[state_id] = &FlagHandler<StateType,Flag>::flag_true;
            }
            else
            {
                // false or forward
                typedef typename ::boost::mpl::and_<
                            typename is_composite_state<StateType>::type,
                            typename ::boost::mpl::not_<
                                    typename has_non_forwarding_flag<Flag>::type>::type >::type composite_no_forward;

                helper<StateType>(entries,state_id,::boost::mpl::bool_<composite_no_forward::type::value>());
            }
        }
    };
    // maintains for every flag a static array containing the flag value for every state
    template <class Flag>
    flag_handler* get_entries_for_flag() const
    {
        BOOST_STATIC_CONSTANT(int, max_state = (mp11::mp_size<state_set_mp11>::value));

        static flag_handler flags_entries[max_state];
        // build a state list, but only once
        static flag_handler* flags_entries_ptr =
            (mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, state_set_mp11>>
                            (init_flags<Flag>(flags_entries)),
            flags_entries);
        return flags_entries_ptr;
    }

     // helper to copy the active states attribute
     template <class region_id,int Dummy=0>
     struct region_copy_helper
     {
         static void do_copy(state_machine* self_,state_machine const& rhs)
         {
             self_->m_active_state_ids[region_id::value] = rhs.m_active_state_ids[region_id::value];
             region_copy_helper< ::boost::mpl::int_<region_id::value+1> >::do_copy(self_,rhs);
         }
     };
     template <int Dummy>
     struct region_copy_helper< ::boost::mpl::int_<nr_regions>,Dummy>
     {
         // end of processing
         static void do_copy(state_machine*,state_machine const& ){}
     };
     // copy functions for deep copy (no need of a 2nd version for NoCopy as noncopyable handles it)
     void do_copy (state_machine const& rhs,
              ::boost::msm::back::dummy<0> = 0)
     {
         // deep copy simply assigns the data
         region_copy_helper< ::boost::mpl::int_<0> >::do_copy(this,rhs);
         m_optional_members = rhs.m_optional_members;
         m_history = rhs.m_history;
         m_event_processing = rhs.m_event_processing;
         m_states = rhs.m_states;
         m_running = rhs.m_running;
         // except for the states themselves, which get duplicated

         ::boost::mpl::for_each<state_set_mp11, ::boost::msm::wrap< ::boost::mpl::placeholders::_1> >
                        (copy_helper(this));
     }

     // helper used to call the correct entry method
     // unfortunately in O(number of states in the sub-sm) but should be better than a virtual call
     template<class Event>
     struct entry_helper
     {
         entry_helper(int id,Event const& e,state_machine* self_):
            state_id(id),evt(e),self(self_){}

         template <class StateAndId>
         void operator()(StateAndId const&)
         {
             using State = mp11::mp_first<StateAndId>;
             using Id = mp11::mp_second<StateAndId>;
             BOOST_STATIC_CONSTANT(int, id = (Id::value));
             if (id == state_id)
             {
                 execute_entry<State>(std::get<Id::value>(self->m_states),evt,*self);
             }
         }
     private:
         int            state_id;
         Event const&   evt;
         state_machine* self;
     };

     // helper used to call the correct exit method
     // unfortunately in O(number of states in the sub-sm) but should be better than a virtual call
     template<class Event>
     struct exit_helper
     {
         exit_helper(int id,Event const& e,state_machine* self_):
            state_id(id),evt(e),self(self_){}

         template <class StateAndId>
         void operator()(StateAndId const&)
         {
             using State = mp11::mp_first<StateAndId>;
             using Id = mp11::mp_second<StateAndId>;
             BOOST_STATIC_CONSTANT(int, id = (Id::value));
             if (id == state_id)
             {
                 execute_exit<State>(std::get<Id::value>(self->m_states),evt,*self);
             }
         }
     private:
         int            state_id;
         Event const&   evt;
         state_machine* self;
     };

     // start for states machines which are themselves embedded in other state machines (composites)
     template <class Event>
     void internal_start(Event const& incomingEvent)
     {
         m_running = true;
         for (size_t region_id=0; region_id<nr_regions; region_id++)
         {
             //forward the event for handling by sub state machines
             mp11::mp_for_each<state_map_mp11>
                 (entry_helper<Event>(m_active_state_ids[region_id],incomingEvent,this));
         }
         // give a chance to handle an anonymous (eventless) transition
         try_process_completion_event(EVENT_SOURCE_DEFAULT, true);
     }

     template <class StateType>
     struct find_region_id
     {
         template <int region,int Dummy=0>
         struct In
         {
             enum {region_index=region};
         };
        enum {region_index = In<StateType::zone_index>::region_index };
     };
     // helper used to set the correct state as active state upon entry into a fsm
     struct direct_event_start_helper
     {
         direct_event_start_helper(state_machine* self_):self(self_){}
         // this variant is for the standard case, entry due to activation of the containing FSM
         template <class EventType,class FsmType>
         typename ::boost::disable_if<typename has_direct_entry<EventType>::type,void>::type
             operator()(EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<0> = 0)
         {
             (static_cast<FrontEnd*>(self))->on_entry(evt,fsm);
             self->internal_start(evt);
         }

         // this variant is for the direct entry case (just one entry, not a sequence of entries)
         template <class EventType,class FsmType>
         typename ::boost::enable_if<
             typename ::boost::mpl::and_<
                        typename ::boost::mpl::not_< typename is_pseudo_entry<
                                    typename EventType::active_state>::type >::type,
                        typename ::boost::mpl::and_<typename has_direct_entry<EventType>::type,
                                                    typename ::boost::mpl::not_<typename ::boost::mpl::is_sequence
                                                            <typename EventType::active_state>::type >::type
                                                    >::type>::type,void
                                  >::type
         operator()(EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<1> = 0)
         {
             (static_cast<FrontEnd*>(self))->on_entry(evt,fsm);
             int state_id = get_state_id<typename EventType::active_state::wrapped_entry>();
             BOOST_STATIC_ASSERT(find_region_id<typename EventType::active_state::wrapped_entry>::region_index >= 0);
             BOOST_STATIC_ASSERT(find_region_id<typename EventType::active_state::wrapped_entry>::region_index < nr_regions);
             // just set the correct zone, the others will be default/history initialized
             self->m_active_state_ids[find_region_id<typename EventType::active_state::wrapped_entry>::region_index] = state_id;
             self->internal_start(evt.m_event);
         }

         // this variant is for the fork entry case (a sequence on entries)
         template <class EventType,class FsmType>
         typename ::boost::enable_if<
             typename ::boost::mpl::and_<
                    typename ::boost::mpl::not_<
                                    typename is_pseudo_entry<typename EventType::active_state>::type >::type,
                    typename ::boost::mpl::and_<typename has_direct_entry<EventType>::type,
                                                typename ::boost::mpl::is_sequence<
                                                                typename EventType::active_state>::type
                                                >::type>::type,void
                                >::type
         operator()(EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<2> = 0)
         {
             (static_cast<FrontEnd*>(self))->on_entry(evt,fsm);
             ::boost::mpl::for_each<typename EventType::active_state,
                                    ::boost::msm::wrap< ::boost::mpl::placeholders::_1> >
                                                        (fork_helper<EventType>(self,evt));
             // set the correct zones, the others (if any) will be default/history initialized
             self->internal_start(evt.m_event);
         }

         // this variant is for the pseudo state entry case
         template <class EventType,class FsmType>
         typename ::boost::enable_if<
             typename is_pseudo_entry<typename EventType::active_state >::type,void
                                    >::type
         operator()(EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<3> = 0)
         {
             // entry on the FSM
             (static_cast<FrontEnd*>(self))->on_entry(evt,fsm);
             int state_id = get_state_id<typename EventType::active_state::wrapped_entry>();
             BOOST_STATIC_ASSERT(find_region_id<typename EventType::active_state::wrapped_entry>::region_index >= 0);
             BOOST_STATIC_ASSERT(find_region_id<typename EventType::active_state::wrapped_entry>::region_index < nr_regions);
             // given region starts with the entry pseudo state as active state
             self->m_active_state_ids[find_region_id<typename EventType::active_state::wrapped_entry>::region_index] = state_id;
             self->internal_start(evt.m_event);
             // and we process the transition in the zone of the newly active state
             // (entry pseudo states are, according to UML, a state connecting 1 transition outside to 1 inside
             self->process_event(evt.m_event);
         }
     private:
         // helper for the fork case, does almost like the direct entry
         state_machine* self;
         template <class EventType>
         struct fork_helper
         {
             fork_helper(state_machine* self_,EventType const& evt_):
                helper_self(self_),helper_evt(evt_){}
             template <class StateType>
             void operator()( ::boost::msm::wrap<StateType> const& )
             {
                 int state_id = get_state_id<typename StateType::wrapped_entry>();
                 BOOST_STATIC_ASSERT(find_region_id<typename StateType::wrapped_entry>::region_index >= 0);
                 BOOST_STATIC_ASSERT(find_region_id<typename StateType::wrapped_entry>::region_index < nr_regions);
                 helper_self->m_active_state_ids[find_region_id<typename StateType::wrapped_entry>::region_index] = state_id;
             }
         private:
             state_machine*     helper_self;
             EventType const&   helper_evt;
         };
     };

     // entry/exit for states machines which are themselves embedded in other state machines (composites)
     template <class Event,class FsmType>
     void do_entry(Event const& incomingEvent,FsmType& fsm)
     {
        // by default we activate the history/init states, can be overwritten by direct_event_start_helper
        for (size_t region_id=0; region_id<nr_regions; region_id++)
        {
             m_active_state_ids[region_id] =
                 m_history.history_entry(incomingEvent)[region_id];
        }
        // block immediate handling of events
        m_event_processing = true;
        // if the event is generating a direct entry/fork, set the current state(s) to the direct state(s)
        direct_event_start_helper(this)(incomingEvent,fsm);
        // handle messages which were generated and blocked in the init calls
        m_event_processing = false;
        // look for deferred events waiting
        try_process_deferred_events(true);
        try_process_queued_events();
     }
     template <class Event,class FsmType>
     void do_exit(Event const& incomingEvent,FsmType& fsm)
     {
        // first recursively exit the sub machines
        // forward the event for handling by sub state machines
        for (size_t region_id=0; region_id<nr_regions; region_id++)
        {
             mp11::mp_for_each<state_map_mp11>
                 (exit_helper<Event>(m_active_state_ids[region_id],incomingEvent,this));
        }
        // then call our own exit
        (static_cast<FrontEnd*>(this))->on_exit(incomingEvent,fsm);
        // give the history a chance to handle this (or not).
        m_history.history_exit(this->m_active_state_ids);
        // history decides what happens with deferred events
        if (!m_history.process_deferred_events(incomingEvent))
        {
            if constexpr (deferred_events_member::value)
            {
                get_deferred_events_queue().clear();
            }
        }
     }

    // the IBM and VC<8 compilers seem to have problems with the friend declaration of dispatch_table
#if defined (__IBMCPP__) || (defined(_MSC_VER) && (_MSC_VER < 1400))
     public:
#endif
    // no transition for event.
    template <class Event>
    static HandledEnum call_no_transition(state_machine& , int , int , Event const& )
    {
        return HANDLED_FALSE;
    }
    // no transition for event for internal transitions (not an error).
    template <class Event>
    static HandledEnum call_no_transition_internal(state_machine& , int , int , Event const& )
    {
        //// reject to give others a chance to handle
        //return HANDLED_GUARD_REJECT;
        return HANDLED_FALSE;
    }
    // clang seems to have a problem with the defer_transition indirection in preprocess_state
#if defined (__clang__ )
     public:
#endif
    // called for deferred events. Address set in the dispatch_table at init
    template <class Event>
    static HandledEnum defer_transition(state_machine& fsm, int , int , Event const& e)
    {
        fsm.defer_event(e);
        return HANDLED_DEFERRED;
    }
    // called for completion events. Default address set in the dispatch_table at init
    // prevents no-transition detection for completion events
    template <class Event>
    static HandledEnum default_eventless_transition(state_machine&, int, int , Event const&)
    {
        return HANDLED_FALSE;
    }
#if defined (__IBMCPP__) || (defined(_MSC_VER) && (_MSC_VER < 1400) || defined(__clang__ ))
     private:
#endif
    // helper function. In cases where the event is wrapped (target is a direct entry states)
    // we want to send only the real event to on_entry, not the wrapper.
    template <class EventType>
    static
    typename boost::enable_if<typename has_direct_entry<EventType>::type,typename EventType::contained_event const& >::type
    remove_direct_entry_event_wrapper(EventType const& evt,boost::msm::back::dummy<0> = 0)
    {
        return evt.m_event;
    }
    template <class EventType>
    static typename boost::disable_if<typename has_direct_entry<EventType>::type,EventType const& >::type
    remove_direct_entry_event_wrapper(EventType const& evt,boost::msm::back::dummy<1> = 0)
    {
        // identity. No wrapper
        return evt;
    }
    // calls the entry/exit or on_entry/on_exit depending on the state type
    // (avoids calling virtually)
    // variant for FSMs
    template <class StateType,class EventType,class FsmType>
    static
        typename boost::enable_if<typename is_composite_state<StateType>::type,void >::type
        execute_entry(StateType& astate,EventType const& evt,FsmType& fsm,boost::msm::back::dummy<0> = 0)
    {
        // calls on_entry on the fsm then handles direct entries, fork, entry pseudo state
        astate.do_entry(evt,fsm);
    }
    // variant for states
    template <class StateType,class EventType,class FsmType>
    static
        typename ::boost::disable_if<
            typename ::boost::mpl::or_<typename is_composite_state<StateType>::type,
                                       typename is_pseudo_exit<StateType>::type >::type,void >::type
    execute_entry(StateType& astate,EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<1> = 0)
    {
        // simple call to on_entry
        astate.on_entry(remove_direct_entry_event_wrapper(evt),fsm);
    }
    // variant for exit pseudo states
    template <class StateType,class EventType,class FsmType>
    static
        typename ::boost::enable_if<typename is_pseudo_exit<StateType>::type,void >::type
    execute_entry(StateType& astate,EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<2> = 0)
    {
        // calls on_entry on the state then forward the event to the transition which should be defined inside the
        // contained fsm
        astate.on_entry(evt,fsm);
        astate.forward_event(evt);
    }
    template <class StateType,class EventType,class FsmType>
    static
        typename ::boost::enable_if<typename is_composite_state<StateType>::type,void >::type
    execute_exit(StateType& astate,EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<0> = 0)
    {
        astate.do_exit(evt,fsm);
    }
    template <class StateType,class EventType,class FsmType>
    static
        typename ::boost::disable_if<typename is_composite_state<StateType>::type,void >::type
    execute_exit(StateType& astate,EventType const& evt,FsmType& fsm, ::boost::msm::back::dummy<1> = 0)
    {
        // simple call to on_exit
        astate.on_exit(evt,fsm);
    }

    // helper allowing special handling of direct entries / fork
    template <class StateType,class TargetType,class EventType,class FsmType>
    static
        typename ::boost::disable_if<
            typename ::boost::mpl::or_<typename has_explicit_entry_state<TargetType>::type,
                                       ::boost::mpl::is_sequence<TargetType> >::type,void>::type
    convert_event_and_execute_entry(StateType& astate,EventType const& evt, FsmType& fsm, ::boost::msm::back::dummy<1> = 0)
    {
        // if the target is a normal state, do the standard entry handling
        execute_entry<StateType>(astate,evt,fsm);
    }
    template <class StateType,class TargetType,class EventType,class FsmType>
    static
        typename ::boost::enable_if<
            typename ::boost::mpl::or_<typename has_explicit_entry_state<TargetType>::type,
                                       ::boost::mpl::is_sequence<TargetType> >::type,void >::type
    convert_event_and_execute_entry(StateType& astate,EventType const& evt, FsmType& fsm, ::boost::msm::back::dummy<0> = 0)
    {
        // for the direct entry, pack the event in a wrapper so that we handle it differently during fsm entry
        execute_entry(astate,direct_entry_event<TargetType,EventType>(evt),fsm);
    }

    // initializes the SM
    template <class TRootSm>
    void init(TRootSm& root_sm)
    {
        if constexpr (!std::is_same_v<RootSm, no_root_sm>)
        {
            static_assert(
                std::is_same_v<TRootSm, RootSm>,
                "The configured root_sm must match the used one."
            );
        }
        m_root_sm = static_cast<void*>(&root_sm);

        visit(
            [&root_sm](auto& state)
            {
                using State = std::remove_reference_t<decltype(state)>;

                if constexpr (is_pseudo_exit<State>::value)
                {
                    state.set_forward_fct(
                        [&root_sm](typename State::event const& event)
                        {
                            return root_sm.process_event(event);
                        }
                    );
                }

                if constexpr (is_composite_state<State>::value)
                {
                    // TODO:
                    // All configs must be identical.
                    static_assert(
                        std::is_same_v<CompilePolicy, typename State::CompilePolicy>,
                        "All compile policies must be identical."
                    );
                    state.init(root_sm);
                }
            },
            all_states
        );
    }

private:
    template<class Fsm, class Compile>
    friend class dispatch_table;

    friend class end_interrupt_event_helper;

    template <class StateType,class Enable=void>
    struct msg_queue_helper
    {
    public:
        msg_queue_helper():m_events_queue(){}
        events_queue_t              m_events_queue;
    };
    template <class StateType>
    struct msg_queue_helper<StateType,
        typename ::boost::enable_if<typename is_no_message_queue<StateType>::type >::type>
    {
    };

    // Dispatch table for visitors.
    template <typename Visitor>
    class visitor_dispatch_table
    {
    public:
        visitor_dispatch_table()
        {
            using state_identities = mp11::mp_transform<mp11::mp_identity, state_set_mp11>;
            mp11::mp_for_each<state_identities>(init_helper{m_cells});
        }

        static void dispatch(state_machine& fsm, int index, Visitor&& visitor)
        {
            instance().m_cells[index](fsm, std::forward<Visitor>(visitor));
        }

    private:
        using visitor_cell = void (*)(state_machine&, Visitor&&);

        class init_helper
        {
          public:
            init_helper(visitor_cell* cells) : m_cells(cells) { }

            template <typename State>
            void operator()(mp11::mp_identity<State>)
            {
                m_cells[get_state_id<State>()] = &accept<State>;
            }

            template<typename State>
            static void accept(state_machine& fsm, Visitor&& visitor)
            {
                State& state = fsm.get_state<State>();
                std::invoke(std::forward<Visitor>(visitor), state);

                if constexpr (is_composite_state<State>::value)
                {
                    state.visit(std::forward<Visitor>(visitor));
                }
            }

          private:
            visitor_cell* m_cells;
        };

        static visitor_dispatch_table& instance()
        {
            static visitor_dispatch_table instance;
            return instance;
        }

        visitor_cell m_cells[mp11::mp_size<state_set_mp11>::value];
    };

    struct optional_members :
        events_queue_member,
        deferred_events_member,
        context_member
    {
        template <typename T>
        typename T::type& get()
        {
            return static_cast<T*>(this)->instance;
        }
        template <typename T>
        const typename T::type& get() const
        {
            return static_cast<const T*>(this)->instance;
        }
    };

    // data members
    active_state_ids_t              m_active_state_ids;
    optional_members                m_optional_members;
    concrete_history                m_history{};
    bool                            m_event_processing{false};
    void*                           m_root_sm{nullptr};
    substate_list                   m_states{};
    bool                            m_running{false};
};

}}} // boost::msm::backmp11

#endif //BOOST_MSM_BACKMP11_STATE_MACHINE_H
