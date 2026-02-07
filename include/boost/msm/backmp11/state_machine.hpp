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

#include <array>
#include <cstdint>
#include <exception>
#include <functional>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/mp11.hpp>

#include <boost/msm/active_state_switching_policies.hpp>
#include <boost/msm/row_tags.hpp>
#include <boost/msm/backmp11/detail/history_impl.hpp>
#include <boost/msm/backmp11/detail/favor_runtime_speed.hpp>
#include <boost/msm/backmp11/detail/state_visitor.hpp>
#include <boost/msm/backmp11/detail/transition_table.hpp>
#include <boost/msm/backmp11/common_types.hpp>
#include <boost/msm/backmp11/state_machine_config.hpp>

namespace boost { namespace msm { namespace backmp11
{

// Check whether a state is a composite state.
using detail::is_composite;

// flag handling
using flag_or = std::logical_or<bool>;
using flag_and = std::logical_and<bool>;

namespace detail
{

template <typename Flag, typename BinaryOp>
struct is_flag_active_visitor;
template <typename Flag>
struct is_flag_active_visitor<Flag, flag_or>
{
    template <typename State>
    using predicate = mp11::mp_or<
        mp11::mp_contains<get_flag_list<State>, Flag>,
        has_state_machine_tag<State>>;

    template <typename State>
    void operator()(const State&)
    {
        result |= mp11::mp_contains<get_flag_list<State>, Flag>::value;
    }

    bool result{false};
};
template <typename Flag>
struct is_flag_active_visitor<Flag, flag_and>
{
    template <typename State>
    using predicate = mp11::mp_or<
        mp11::mp_not<mp11::mp_contains<get_flag_list<State>, Flag>>,
        has_state_machine_tag<State>>;

    template <typename State>
    void operator()(const State&)
    {
        result &= mp11::mp_contains<get_flag_list<State>, Flag>::value;
    }

    bool result{true};
};

template <typename State>
struct is_state_active_visitor
{
    template <typename StateToCheck>
    using predicate = mp11::mp_or<std::is_same<State, StateToCheck>,
                                  has_state_machine_tag<StateToCheck>>;

    template <typename StateToCheck>
    void operator()(const StateToCheck&)
    {
    }

    void operator()(const State&)
    {
        result = true;
    }

    bool result{false};
};

template <
    class FrontEnd,
    class Config,
    class Derived
>
class state_machine_base : public FrontEnd
{
    static_assert(
        is_composite<FrontEnd>::value,
        "FrontEnd must be a composite state");
    static_assert(
        is_config<Config>::value,
        "Config must be an instance of state machine config");

  public:
    using config_t = Config;
    using root_sm_t = typename config_t::root_sm;
    using context_t = typename config_t::context;
    using front_end_t = FrontEnd;
    using derived_t = Derived;

    // Event that describes the SM is starting.
    // Used when the front-end does not define an initial_event.
    struct starting {};
    // Event that describes the SM is stopping.
    // Used when the front-end does not define a final_event.
    struct stopping {};

    // Wrapper for an exit pseudostate,
    // which upper SMs can use to connect to it.
    template <class ExitPseudostate>
    struct exit_pt : public ExitPseudostate
    {
        // tags
        struct internal
        {
            using tag = exit_pseudostate_be_tag;
        };
        using state = ExitPseudostate;
        using owner = derived_t;
        using event = typename ExitPseudostate::event;
        using forward_function = std::function<process_result(event const&)>;

        // forward event to the higher-level FSM
        template <class ForwardEvent>
        void forward_event(ForwardEvent const& incomingEvent)
        {
            static_assert(std::is_convertible_v<ForwardEvent, event>,
                          "ForwardEvent must be convertible to exit pseudostate's event");
            // Call if handler set. If not, this state is simply a terminate state.
            if (m_forward)
            {
                m_forward(incomingEvent);
            }
        }
        void set_forward_fct(forward_function fct)
        {
            m_forward = fct;
        }
        exit_pt() = default;
        // by assignments, we keep our forwarding functor unchanged as our containing SM did not change
        template <class RHS>
        exit_pt(RHS&) : m_forward()
        {
        }
        exit_pt<ExitPseudostate>& operator=(const exit_pt<ExitPseudostate>&)
        {
            return *this;
        }

      private:
        forward_function          m_forward;
    };

    // Wrapper for an entry pseudostate,
    // which upper SMs can use to connect to it.
    template <class EntryPseudostate>
    struct entry_pt : public EntryPseudostate
    {
        // tags
        struct internal
        {
            using tag = entry_pseudostate_be_tag;
        };

        using state = EntryPseudostate;
        using owner = derived_t;
    };

    // Wrapper for a direct entry,
    // which upper SMs can use to connect to it.
    template <class State>
    struct direct : public State
    {
        // tags
        struct internal
        {
            using tag = explicit_entry_be_tag;
        };
        using state = State;
        using owner = derived_t;
    };

    struct internal
    {
        using tag = state_machine_tag;

        using initial_states = to_mp_list_t<typename front_end_t::initial_state>;
        static constexpr int nr_regions = mp11::mp_size<initial_states>::value;

        using state_set = generate_state_set<state_machine_base>;
        using submachines = mp11::mp_copy_if<state_set, is_composite>;
    };

    using states_t = mp11::mp_rename<typename internal::state_set, std::tuple>;

  protected:
    template <typename T>
    using event_container = typename config_t::template event_container<T>;
    using event_container_t =
        event_container<basic_unique_ptr<event_occurrence>>;

    struct event_pool_t
    {
        event_container_t events;
        uint16_t cur_seq_cnt{};
    };

    using event_pool_member = optional_instance<
        event_pool_t,
        !std::is_same_v<event_container<void>, no_event_container<void>>>;

    template <bool C = event_pool_member::value,
              typename = std::enable_if_t<C>>
    event_pool_t& get_event_pool()
    {
        return m_optional_members.template get<event_pool_member>();
    }

    template <bool C = event_pool_member::value,
              typename = std::enable_if_t<C>>
    const event_pool_t& get_event_pool() const
    {
        return m_optional_members.template get<event_pool_member>();
    }

  private:
    using state_set = typename internal::state_set;
    static constexpr int nr_regions = internal::nr_regions;
    using active_state_ids_t = std::array<int, nr_regions>;
    using initial_state_identities = mp11::mp_transform<mp11::mp_identity, typename internal::initial_states>;
    using compile_policy = typename config_t::compile_policy;
    using compile_policy_impl = detail::compile_policy_impl<compile_policy>;

    template<typename T>
    using get_active_state_switch_policy = typename T::active_state_switch_policy;
    using active_state_switching =
        mp11::mp_eval_or<active_state_switch_after_entry,
                         get_active_state_switch_policy, front_end_t>;

    template <class, class, class>
    friend class state_machine_base;

    template <typename StateMachine>
    friend struct transition_table_impl;

    template <typename Policy>
    friend struct detail::compile_policy_impl;

    template <typename, typename, visit_mode, typename, template <typename> typename...>
    friend class state_visitor_impl;

    // Allow access to private members for serialization.
    // WARNING:
    // No guarantee is given on the private member layout.
    // Future changes may break existing serializer implementations.
    template<typename T, typename A0, typename A1, typename A2>
    friend void serialize(T&, state_machine_base<A0, A1, A2>&);

    template <typename T>
    using get_initial_event = typename T::initial_event;
    using fsm_initial_event =
        mp11::mp_eval_or<starting, get_initial_event, front_end_t>;

    template <typename T>
    using get_final_event = typename T::final_event;
    using fsm_final_event =
        mp11::mp_eval_or<stopping, get_final_event, front_end_t>;
    
    using state_map = generate_state_map<state_set>;
    using history_impl = detail::history_impl<typename front_end_t::history, nr_regions>;

    using deferring_states = mp11::mp_copy_if<state_set, has_deferred_events>;

    using context_member =
        optional_instance<context_t*,
                          !std::is_same_v<context_t, no_context> &&
                              (std::is_same_v<root_sm_t, no_root_sm> ||
                               std::is_same_v<root_sm_t, derived_t>)>;

    // Visit states with a compile-time filter (reduces template instantiations).
    // Kept private for now, because the API of this method is not stable yet
    // and this optimization is likely not needed to be available in the public API.
    template <visit_mode Mode, template <typename> typename... Predicates, typename Visitor>
    void visit_if(Visitor&& visitor)
    {
        using state_visitor = state_visitor<state_machine_base, Visitor, Mode, Predicates...>;
        state_visitor::visit(*this, std::forward<Visitor>(visitor));
    }
    template <visit_mode Mode, template <typename> typename... Predicates, typename Visitor>
    void visit_if(Visitor&& visitor) const
    {
        using state_visitor = state_visitor<const state_machine_base, Visitor, Mode, Predicates...>;
        state_visitor::visit(*this, std::forward<Visitor>(visitor));
    }
    
  public:
    // Construct and forward constructor arguments to the front-end.
    template <typename... Args>
    state_machine_base(Args&&... args)
        : front_end_t(std::forward<Args>(args)...)
    {
        static_assert(
            std::is_base_of_v<state_machine_base, derived_t>,
            "Derived must inherit from state_machine");
        if constexpr (!std::is_same_v<context_t, no_context>)
        {
            static_assert(
                std::is_constructible_v<derived_t, context_t&>,
                "Derived must inherit the base class constructors");
        }
        if constexpr (std::is_same_v<root_sm_t, no_root_sm> ||
                      std::is_same_v<root_sm_t, derived_t>)
        {
            // create states
            init(self());
        }
        reset_active_state_ids();
    }

    // Construct with a context and forward further constructor arguments to the front-end.
    template <bool C = context_member::value,
              typename = std::enable_if_t<C>,
              typename... Args>
    state_machine_base(context_t& context, Args&&... args)
        : state_machine_base(std::forward<Args>(args)...)
        {
            m_optional_members.template get<context_member>() = &context;
            if constexpr (std::is_same_v<root_sm_t, no_root_sm>)
            {
                visit_if<visit_mode::all_recursive, has_state_machine_tag>(
                    [&context](auto &state_machine)
                    {
                        state_machine.m_optional_members.template get<context_member>() = &context;
                    });
            }
        }
    
    // Copy constructor.
    state_machine_base(state_machine_base const& rhs)
        : front_end_t(rhs)
    {
        if constexpr (std::is_same_v<root_sm_t, no_root_sm> ||
                      std::is_same_v<root_sm_t, derived_t>)
        {
            // create states
            init(self());
        }
        // Copy all members except the root sm pointer.
        m_active_state_ids = rhs.m_active_state_ids;
        m_optional_members = rhs.m_optional_members;
        m_history = rhs.m_history;
        m_event_processing = rhs.m_event_processing;
        m_states = rhs.m_states;
        m_running = rhs.m_running;
    }

    // Copy assignment operator.
    state_machine_base& operator= (state_machine_base const& rhs)
    {
        if (this != &rhs)
        {
            front_end_t::operator=(rhs);
            // Copy all members except the root sm pointer.
            m_active_state_ids = rhs.m_active_state_ids;
            m_optional_members = rhs.m_optional_members;
            m_history = rhs.m_history;
            m_event_processing = rhs.m_event_processing;
            m_states = rhs.m_states;
            m_running = rhs.m_running;
        }
       return *this;
    }

    // Start the state machine (calls entry of the initial state(s)).
    void start()
    {
        // Assert for a case where root sm was not set up correctly
        // after construction.
        if constexpr (!std::is_same_v<typename Config::root_sm, no_root_sm>)
        {
            BOOST_ASSERT_MSG(&(this->get_root_sm()),
            "Root sm must be passed as Derived and configured as root_sm");
        }
        start(fsm_initial_event{});
    }

    // Start the state machine
    // (calls entry of the initial state(s) with initial_event).
    template <class Event>
    void start(Event const& initial_event)
    {
        if (!m_running)
        {
            on_entry(initial_event, get_fsm_argument());
        }
    }

    // Stop the state machine (calls exit of the current state(s)).
    void stop()
    {
        stop(fsm_final_event{});
    }

    // Stop the state machine
    // (calls exit of the current state(s) with final_event).
    template <class Event>
    void stop(Event const& final_event)
    {
        if (m_running)
        {
            on_exit(final_event, get_fsm_argument());
            m_running = false;
        }
    }

    // Main function to process events.
    template<class Event>
    process_result process_event(Event const& event)
    {
        return process_event_internal(
            compile_policy_impl::normalize_event(event),
            process_info::direct_call);
    }

    // Try to process pending event occurrences in the event pool,
    // with an optional limit for the max no. of events that shall be processed.
    // Returns the no. of processed events.
    template <bool C = event_pool_member::value,
              typename = std::enable_if_t<C>>
    inline size_t process_event_pool(size_t max_events = SIZE_MAX)
    {
        if (get_event_pool().events.empty())
        {
            return 0;
        }
        return do_process_event_pool(max_events);
    }

    // Enqueues an event in the message queue.
    // Call process_queued_events to process all queued events.
    // Be careful if you do this during event processing, the event will be processed immediately
    // and not kept in the queue.
    template <class Event,
              bool C = event_pool_member::value,
              typename = std::enable_if_t<C>>
    [[deprecated ("Use defer_event(event) instead")]]
    void enqueue_event(Event const& event)
    {
        compile_policy_impl::defer_event(
            *this, compile_policy_impl::normalize_event(event), !m_event_processing);
    }

    // Process all queued events.
    template <bool C = event_pool_member::value,
              typename = std::enable_if_t<C>>
    [[deprecated ("Use process_event_pool() instead")]]
    void process_queued_events()
    {
        process_event_pool();
    }

    // Process a single queued event.
    template <bool C = event_pool_member::value,
              typename = std::enable_if_t<C>>
    [[deprecated ("Use process_event_pool(1) instead")]]
    void process_single_queued_event()
    {
        process_event_pool(1);
    }

    // Get the context of the state machine.
    template <bool C = !std::is_same_v<context_t, no_context>,
              typename = std::enable_if_t<C>>
    context_t& get_context()
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

    // Get the context of the state machine.
    template <bool C = !std::is_same_v<context_t, no_context>,
              typename = std::enable_if_t<C>>
    const context_t& get_context() const
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

    // Getter that returns the currently active state ids of the FSM.
    const active_state_ids_t& get_active_state_ids() const
    {
        return m_active_state_ids;
    }

    // Get the root sm.
    template <typename T = root_sm_t, 
              typename = std::enable_if_t<!std::is_same_v<T, no_root_sm>>>
    root_sm_t& get_root_sm()
    {
        return *static_cast<root_sm_t*>(m_root_sm);
    }
    // Get the root sm.
    template <typename T = root_sm_t, 
              typename = std::enable_if_t<!std::is_same_v<T, no_root_sm>>>
    const root_sm_t& get_root_sm() const
    {
        return *static_cast<const root_sm_t*>(m_root_sm);
    }

    // Return the id of a state in the sm.
    template<typename State>
    static constexpr int get_state_id(const State&)
    {
        static_assert(
            mp11::mp_map_contains<state_map, State>::value,
            "The state must be contained in the state machine");
        return detail::get_state_id<state_map, State>::value;
    }
    // Return the id of a state in the sm.
    template<typename State>
    static constexpr int get_state_id()
    {
        static_assert(
            mp11::mp_map_contains<state_map, State>::value,
            "The state must be contained in the state machine");
        return detail::get_state_id<state_map, State>::value;
    }

    // True if the sm is used in another sm.
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
    // Get a state.
    template <class State>
    const State& get_state() const
    {
        return std::get<std::remove_reference_t<State>>(m_states);
    }

    // Visit the states (only active states, recursive).
    template <typename Visitor>
    void visit(Visitor&& visitor)
    {
        visit<visit_mode::active_recursive>(std::forward<Visitor>(visitor));
    }

    // Visit the states (only active states, recursive).
    template <typename Visitor>
    void visit(Visitor&& visitor) const
    {
        visit<visit_mode::active_recursive>(std::forward<Visitor>(visitor));
    }

    // Visit the states.
    // How to traverse is selected with visit_mode.
    template <visit_mode Mode, typename Visitor>
    void visit(Visitor&& visitor)
    {
        visit_if<Mode>(std::forward<Visitor>(visitor));
    }

    // Visit the states.
    // How to traverse is selected with visit_mode.
    template <visit_mode Mode, typename Visitor>
    void visit(Visitor&& visitor) const
    {
        visit_if<Mode>(std::forward<Visitor>(visitor));
    }

    // Check whether a state is currently active.
    template <typename State>
    bool is_state_active() const
    {
        using visitor_t = is_state_active_visitor<State>;
        visitor_t visitor;
        visit_if<visit_mode::active_recursive,
                 visitor_t::template predicate>(visitor);
        return visitor.result;
    }

    // Check if a flag is active, using the BinaryOp as folding function.
    template <typename Flag, typename BinaryOp = flag_or>
    bool is_flag_active() const
    {
        using visitor_t = is_flag_active_visitor<Flag, BinaryOp>;
        visitor_t visitor;
        visit_if<visit_mode::active_recursive,
                 visitor_t::template predicate>(visitor);
        return visitor.result;
    }

    // Puts the event into the event pool for later processing.
    // If the deferral takes place while the state machine is processing,
    // the event will be evaluated for dispatch from the next processing cycle.
    template <
        class Event,
        bool C = event_pool_member::value,
        typename = std::enable_if_t<C>>
    void defer_event(Event const& event)
    {
        compile_policy_impl::defer_event(
            *this, compile_policy_impl::normalize_event(event), m_event_processing);
    }

  protected:
    static_assert(std::is_same_v<typename config_t::fsm_parameter, local_transition_owner> ||
                    (std::is_same_v<typename config_t::fsm_parameter, typename config_t::root_sm> &&
                     !std::is_same_v<typename config_t::root_sm, no_root_sm>),
                  "fsm_parameter must be local_transition_owner or root_sm"
                 );
    using fsm_parameter_t = mp11::mp_if_c<
        std::is_same_v<typename config_t::fsm_parameter, local_transition_owner>,
        derived_t,
        typename config_t::root_sm>;

    fsm_parameter_t& get_fsm_argument()
    {
        if constexpr (std::is_same_v<typename config_t::fsm_parameter,
                                     local_transition_owner>)
        {
            return self();
        }
        else
        {
            return get_root_sm();
        }
    }

    // Checks if an event is an end interrupt event.
    template <typename Event>
    bool is_end_interrupt_event(const Event& event) const
    {
        return compile_policy_impl::is_end_interrupt_event(*this, event);
    }

    // Helpers used to reset the state machine.
    void reset_active_state_ids()
    {
       size_t index = 0;
       mp11::mp_for_each<initial_state_identities>(
       [this, &index](auto state_identity)
       {
           using State = typename decltype(state_identity)::type;
           m_active_state_ids[index++] = get_state_id<State>();
       });
       m_history.reset_active_state_ids(m_active_state_ids);
    }

    // Handling of completion transitions.
    template <typename Transition>
    using is_completion_transition =
        has_completion_event<typename Transition::transition_event>;

    void try_process_completion_event(bool handled)
    {
        // MSVC requires a detail prefix.
        using table = detail::transition_table<derived_t>;
        using index = mp11::mp_find_if<table, is_completion_transition>;

        if constexpr (index::value != mp11::mp_size<table>::value)
        {
            if (handled)
            {
                using completion_event =
                    typename mp11::mp_at<table, index>::transition_event;
                process_event(completion_event{});
            }
        }
    }
    
    // Main function used internally to process events.
    // Explicitly not inline, because code size can significantly increase if
    // this method's content is inlined in all existing process_info contexts.
    template <class Event>
    BOOST_NOINLINE process_result process_event_internal(
        Event const& event,
        process_info info)
    {
        // If the state machine has terminate or interrupt flags, check them.
        if constexpr (mp11::mp_any_of<state_set, is_state_blocking>::value)
        {
            // If the state machine is terminated, do not handle any event.
            if (is_flag_active<TerminateFlag>())
            {
                return process_result::HANDLED_TRUE;
            }
            // If the state machine is interrupted, do not handle any event
            // unless the event is the end interrupt event.
            if (is_flag_active<InterruptedFlag>() && !is_end_interrupt_event(event))
            {
                return process_result::HANDLED_TRUE;
            }
        }

        if constexpr (event_pool_member::value)
        {
            if (info != process_info::event_pool)
            {
                // If we are already processing or the event is deferred in the
                // active state configuration, process it later.
                if (m_event_processing ||
                    compile_policy_impl::is_event_deferred(self(), event))
                {
                    compile_policy_impl::defer_event(self(), event, false);
                    return process_result::HANDLED_DEFERRED;
                }

                // Ensure we consider an event
                // that was action-deferred in the last sequence.
                get_event_pool().cur_seq_cnt += 1;
            }
        }
        else
        {
            BOOST_ASSERT_MSG(!m_event_processing,
                             "An event pool must be available to call "
                             "process_event while processing an event");
        }

        // Process the event.
        m_event_processing = true;
        process_result result;
#ifndef BOOST_NO_EXCEPTIONS
        if constexpr (has_no_exception_thrown<front_end_t>::value)
        {
            result = do_process_event(event, info);
        }
        else
        {
            try
            {
                result = do_process_event(event, info);
            }
            catch (std::exception& e)
            {
                // give a chance to the concrete state machine to handle
                this->exception_caught(event, get_fsm_argument(), e);
                result = process_result::HANDLED_FALSE;
            }
        }
#else
        result = do_process_event(event, info);
#endif
        m_event_processing = false;

        // Process completion transitions BEFORE any other event in the
        // pool (UML Standard 2.3 15.3.14)
        try_process_completion_event(result & process_result::HANDLED_TRUE);

        // After handling, look if we have more to process in the event pool
        // (but only if we're not already processing from it).
        if constexpr (event_pool_member::value)
        {
            if (info != process_info::event_pool)
            {
                process_event_pool();
            }
        }

        return result;
    }

  private:
    // Core logic for event processing without exceptions, queues, etc.
    template<class Event>
    process_result do_process_event(Event const& event, process_info info)
    {
        using dispatch_table =
            typename compile_policy_impl::template dispatch_table<derived_t,
                                                                  Event>;
        process_result result = process_result::HANDLED_FALSE;
        // Dispatch the event to every region.
        for (int region_id = 0; region_id < nr_regions; region_id++)
        {
            result |= dispatch_table::dispatch(
                self(), m_active_state_ids[region_id], event);
        }
        // Dispatch the event to the SM-internal table if it hasn't been consumed yet.
        if (!(result & handled_true_or_deferred))
        {
            result |= dispatch_table::internal_dispatch(self(), event);
        }

        // If the event has not been handled and we have orthogonal zones, then
        // generate an error on every active state.
        // For events coming from upper machines, do not handle
        // but let the upper sm handle the error.
        // Completion events do not produce an error.
        if (!(result || (info == process_info::submachine_call) || compile_policy_impl::is_completion_event(event)))
        {
            for (const auto state_id: m_active_state_ids)
            {
                this->no_transition(event, get_fsm_argument(), state_id);
            }
        }
        return result;
    }

    // Core logic for event pool processing,
    // there must be at least one event in the pool.
    // Explicitly not inline, because code size can significantly increase if
    // this method's content is inlined in all entries and process_event calls.
    template <bool C = event_pool_member::value,
              typename = std::enable_if_t<C>>
    BOOST_NOINLINE size_t do_process_event_pool(size_t max_events = SIZE_MAX)
    {
        event_pool_t& event_pool = get_event_pool();
        auto it = event_pool.events.begin();
        size_t processed_events = 0;
        do
        {
            event_occurrence& event = **it;
            // The event was already processed.
            if (event.marked_for_deletion)
            {
                it = event_pool.events.erase(it);
                continue;
            }

            std::optional<process_result> result =
                event.process(event_pool.cur_seq_cnt);
            // The event has not been dispatched.
            if (!result.has_value())
            {
                it++;
                continue;
            }

            // Consider anything except "only deferred" to be a processed event.
            if (result != process_result::HANDLED_DEFERRED)
            {
                processed_events++;
                if (processed_events == max_events)
                {
                    break;
                }
            }

            // Start from the beginning, we might be able to process
            // events that were deferred before.
            it = event_pool.events.begin();
            // Consider newly deferred events only if
            // the event was not deferred at the same time
            // (required to prevent infinitely processing the same event,
            // if it was handled and at the same time action-deferred
            // in orthogonal regions).
            if (!(*result & process_result::HANDLED_DEFERRED))
            {
                event_pool.cur_seq_cnt += 1;
            }
        } while (it != event_pool.events.end());
        return processed_events;
    }

    template <typename Event>
    class deferred_event : public event_occurrence
    {
      public:
        deferred_event(derived_t& sm, const Event& event, uint16_t seq_cnt)
            : m_sm(sm), m_seq_cnt(seq_cnt), m_event(event)
        {
        }

        std::optional<process_result> process(uint16_t seq_cnt) override
        {
            auto& sm = m_sm;
            if ((m_seq_cnt == seq_cnt) ||
                compile_policy_impl::is_event_deferred(sm, m_event))
            {
                return std::nullopt;
            }
            this->marked_for_deletion = true;
            return sm.process_event_internal(
                m_event,
                process_info::event_pool);
        }

      private:
        derived_t& m_sm;
        const uint16_t m_seq_cnt;
        const Event m_event;
    };

    template <class Event>
    void do_defer_event(const Event& event, bool next_rtc_seq)
    {
        auto& event_pool = get_event_pool();
        const uint16_t seq_cnt = next_rtc_seq ? event_pool.cur_seq_cnt
                                              : event_pool.cur_seq_cnt - 1;
        event_pool.events.push_back(basic_unique_ptr<event_occurrence>{
            new deferred_event<Event>(self(), event, seq_cnt)});
    }

    template <class Event, class Fsm>
    void preprocess_entry(Event const& event, Fsm& fsm)
    {
        m_running = true;
        m_event_processing = true;

        // Call on_entry on this SM first.
        static_cast<front_end_t*>(this)->on_entry(event, fsm);
    }

    void postprocess_entry()
    {
        m_event_processing = false;

        // Process completion transitions BEFORE any other event in the
        // pool (UML Standard 2.3 15.3.14)
        try_process_completion_event(true);

        // After handling, look if we have more to process in the event pool.
        if constexpr (event_pool_member::value)
        {
            process_event_pool();
        }
    }

    template <typename Event>
    class state_entry_visitor
    {
      public:
        state_entry_visitor(derived_t& self, const Event& event)
            : m_self(self), m_event(event)
        {
        }

        template <typename State>
        void operator()(State& state)
        {
            state.on_entry(m_event, m_self.get_fsm_argument());
        }

      private:
        derived_t& m_self;
        const Event& m_event;
    };


    template <class Event, class Fsm>
    void on_entry(Event const& event, Fsm& fsm)
    {
        preprocess_entry(event, fsm);

        // First set all active state ids...
        m_active_state_ids = m_history.on_entry(event);
        // ... then execute each state entry.
        state_entry_visitor<Event> visitor{self(), event};
        if constexpr (std::is_same_v<typename front_end_t::history,
                                     front::no_history>)
        {
            mp11::mp_for_each<initial_state_identities>(
                [this, &visitor](auto state_identity)
                {
                    using State = typename decltype(state_identity)::type;
                    auto& state = this->get_state<State>();
                    visitor(state);
                });
        }
        else
        {
            visit<visit_mode::active_non_recursive>(visitor);
        }

        postprocess_entry();
    }

    template <class TargetStates, class Event, class Fsm>
    void on_explicit_entry(Event const& event, Fsm& fsm)
    {
        preprocess_entry(event, fsm);

        using state_identities =
            mp11::mp_transform<mp11::mp_identity, TargetStates>;
        static constexpr bool all_regions_defined =
            mp11::mp_size<state_identities>::value == nr_regions;

        // First set all active state ids...
        if constexpr (!all_regions_defined)
        {
            m_active_state_ids = m_history.on_entry(event);
        }
        mp11::mp_for_each<state_identities>(            
            [this](auto state_identity)
            {
                using State = typename decltype(state_identity)::type;
                static constexpr int region_id = State::zone_index;
                static_assert(region_id >= 0 && region_id < nr_regions);
                m_active_state_ids[region_id] = get_state_id<State>();
            }
        );
        // ... then execute each state entry.
        state_entry_visitor<Event> visitor{self(), event};
        if constexpr (all_regions_defined)
        {
            mp11::mp_for_each<state_identities>(
                [this, &visitor](auto state_identity)
                {
                    using State = typename decltype(state_identity)::type;
                    auto& state = this->get_state<State>();
                    visitor(state);
                });
        }
        else
        {
            visit<visit_mode::active_non_recursive>(visitor);
        }

        postprocess_entry();
    }

    template <class TargetStates, class Event, class Fsm>
    void on_pseudo_entry(Event const& event, Fsm& fsm)
    {
        on_explicit_entry<TargetStates>(event, fsm);

        // Execute the second part of the compound transition.
        process_event(event);
    }

    template <class Event, class Fsm>
    void on_exit(Event const& event, Fsm& fsm)
    {
        // First exit the substates.
        visit<visit_mode::active_non_recursive>(
            [this, &event](auto& state)
            {
                state.on_exit(event, get_fsm_argument());
            }
        );
        // Then call our own exit.
        (static_cast<front_end_t*>(this))->on_exit(event, fsm);
        // Give the history a chance to handle this (or not).
        m_history.on_exit(this->m_active_state_ids);
        // History decides what happens with the event pool.
        if (m_history.clear_event_pool(event))
        {
            if constexpr (event_pool_member::value)
            {
                get_event_pool().events.clear();
            }
        }
    }

    template <typename State>
    using state_requires_init =
        mp11::mp_or<has_exit_pseudostate_be_tag<State>, has_state_machine_tag<State>>;

    // initializes the SM
    template <class TRootSm>
    void init(TRootSm& root_sm)
    {
        if constexpr (!std::is_same_v<root_sm_t, no_root_sm>)
        {
            static_assert(
                std::is_same_v<TRootSm, root_sm_t>,
                "The configured root_sm must match the used one"
            );
            static_assert(
                std::is_same_v<context_t, no_context> ||
                std::is_same_v<context_t, typename TRootSm::context_t>,
                "The configured context must match the root sm's one");
        }
        m_root_sm = static_cast<void*>(&root_sm);

        visit_if<visit_mode::all_non_recursive, state_requires_init>(
            [&root_sm](auto& state)
            {
                using State = std::decay_t<decltype(state)>;

                if constexpr (has_exit_pseudostate_be_tag<State>::value)
                {
                    state.set_forward_fct(
                        [&root_sm](typename State::event const& event)
                        {
                            return root_sm.process_event(event);
                        }
                    );
                }

                if constexpr (has_state_machine_tag<State>::value)
                {
                    static_assert(
                        std::is_same_v<compile_policy,
                                       typename State::compile_policy>,
                        "All compile policies must be identical");
                    state.init(root_sm);
                }
            }
        );
    }

    derived_t& self()
    {
        return *static_cast<derived_t*>(this);
    }

    const derived_t& self() const
    {
        return *static_cast<const derived_t*>(this);
    }

    struct optional_members :
        event_pool_member,
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
    active_state_ids_t   m_active_state_ids;
    optional_members     m_optional_members;
    history_impl         m_history{};
    bool                 m_event_processing{false};
    void*                m_root_sm{nullptr};
    states_t             m_states{};
    bool                 m_running{false};
};

} // detail

/**
 * @brief Back-end for state machines.
 *
 * Can take 1...3 parameters.
 * 
 * @tparam T0 (mandatory) : Front-end
 * @tparam T1 (optional)  : State machine config
 * @tparam T2 (optional)  : Derived class (required when inheriting from state_machine)
 */
template <typename ...T>
class state_machine;

template <class FrontEnd, class Config, class Derived>
class state_machine<FrontEnd, Config, Derived>
    : public detail::state_machine_base<FrontEnd, Config, Derived>
{
    using Base = detail::state_machine_base<FrontEnd, Config, Derived>;
  public:
    using Base::Base;
};

template <class FrontEnd, class Config>
class state_machine<FrontEnd, Config>
    : public detail::state_machine_base<FrontEnd, Config, state_machine<FrontEnd, Config>>
{
    using Base = detail::state_machine_base<FrontEnd, Config, state_machine<FrontEnd, Config>>;
  public:
    using Base::Base;
};

template <class FrontEnd>
class state_machine<FrontEnd>
    : public detail::state_machine_base<FrontEnd, default_state_machine_config, state_machine<FrontEnd>>
{
    using Base = detail::state_machine_base<FrontEnd, default_state_machine_config, state_machine<FrontEnd>>;
  public:
    using Base::Base;
};

}}} // boost::msm::backmp11

#endif //BOOST_MSM_BACKMP11_STATE_MACHINE_H
