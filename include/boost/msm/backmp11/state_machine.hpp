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
#include <exception>
#include <functional>
#include <list>
#include <utility>

#include <boost/assert.hpp>
#include <boost/core/no_exceptions_support.hpp>
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

// Bitmask for process result checks.
static constexpr process_result handled_or_deferred =
    process_result::HANDLED_TRUE | process_result::HANDLED_DEFERRED;

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
    using events_queue_t = typename config_t::template
        queue_container<basic_unique_ptr<enqueued_event>>;

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

    using deferred_events_queue_t = std::list<basic_unique_ptr<deferred_event>>;

    struct deferred_events_t
    {
        deferred_events_queue_t queue;
        size_t cur_seq_cnt;
    };
    using deferring_states = mp11::mp_copy_if<state_set, has_deferred_events>;
    using has_deferring_states =
        mp11::mp_not<mp11::mp_empty<deferring_states>>;
    using deferred_events_member =
        optional_instance<deferred_events_t,
                          has_deferring_states::value ||
                              has_activate_deferred_events<front_end_t>::value>;
    using events_queue_member =
        optional_instance<events_queue_t,
                          !has_no_message_queue<front_end_t>::value>;
    using context_member =
        optional_instance<context_t*,
                          !std::is_same_v<context_t, no_context> &&
                              (std::is_same_v<root_sm_t, no_root_sm> ||
                               std::is_same_v<root_sm_t, derived_t>)>;

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
            init(*static_cast<derived_t*>(this));
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
            init(*static_cast<derived_t*>(this));
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

    // Start the state machine (calls entry of the initial state).
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

    // Start the state machine (calls entry of the initial state with initial_event to on_entry's).
    template <class Event>
    void start(Event const& initial_event)
    {
        if (!m_running)
        {
            on_entry(initial_event, get_fsm_argument());
        }
    }

    // stop the state machine (calls exit of the current state)
    void stop()
    {
        stop(fsm_final_event{});
    }

    // stop the state machine (calls exit of the current state passing finalEvent to on_exit's)
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
            EventSource::EVENT_SOURCE_DIRECT);
    }

  private:
    template <typename Event>
    class enqueued_event_impl : public enqueued_event
    {
      public:
        enqueued_event_impl(state_machine_base& sm, const Event& event)
            : m_sm(sm), m_event(event)
        {
        }

        process_result process() override
        {
            return m_sm.process_event_internal(
                m_event,
                EventSource::EVENT_SOURCE_DIRECT |
                EventSource::EVENT_SOURCE_MSG_QUEUE);
        }

      private:
        state_machine_base& m_sm;
        Event m_event;
    };

  public:
    // Enqueues an event in the message queue.
    // Call process_queued_events to process all queued events.
    // Be careful if you do this during event processing, the event will be processed immediately
    // and not kept in the queue.
    template <class Event,
              bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    void enqueue_event(Event const& event)
    {
        using NormalizedEvent =
            std::decay_t<decltype(compile_policy_impl::normalize_event(event))>;
        get_events_queue().push_back(basic_unique_ptr<enqueued_event>{
            new enqueued_event_impl<NormalizedEvent>(
                *this,
                compile_policy_impl::normalize_event(event))});
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
        basic_unique_ptr<enqueued_event> event = std::move(get_events_queue().front());
        get_events_queue().pop_front();
        event->process();
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

    // Get the events queued for later processing.
    template <bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    events_queue_t& get_events_queue()
    {
        return m_optional_members.template get<events_queue_member>();
    }

    // Get the events queued for later processing.
    template <bool C = events_queue_member::value,
              typename = std::enable_if_t<C>>
    const events_queue_t& get_events_queue() const
    {
       return m_optional_members.template get<events_queue_member>();
    }

    // Get the deferred events queued for later processing.
    template <bool C = deferred_events_member::value,
              typename = std::enable_if_t<C>>
    deferred_events_queue_t& get_deferred_events_queue()
    {
        return get_deferred_events().queue;
    }

    // Get the deferred events queued for later processing.
    template <bool C = deferred_events_member::value,
              typename = std::enable_if_t<C>>
    const deferred_events_queue_t& get_deferred_events_queue() const
    {
        return get_deferred_events().queue;
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


    // Puts the given event into the deferred events queue.
    template <
        class Event,
        bool C = deferred_events_member::value,
        typename = std::enable_if_t<C>>
    void defer_event(Event const& event)
    {
        compile_policy_impl::defer_event(*this, event);
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
            return *static_cast<derived_t*>(this);
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
    
    // handling of deferred events
    void try_process_deferred_events()
    {
        if constexpr (deferred_events_member::value)
        {
            deferred_events_t& deferred_events = get_deferred_events();
            if (deferred_events.queue.empty())
            {
                return;
            }

            active_state_ids_t active_state_ids = m_active_state_ids;
            // Iteratively process all of the events within the deferred
            // queue up to (but not including) newly deferred events.
            auto it = deferred_events.queue.begin();
            do
            {
                if (deferred_events.cur_seq_cnt == (*it)->get_seq_cnt())
                {
                    return;
                }
                if ((*it)->is_deferred())
                {
                    it = std::next(it);
                }
                else
                {
                    basic_unique_ptr<deferred_event> deferred_event = std::move(*it);
                    it = deferred_events.queue.erase(it);
                    const process_result result = deferred_event->process();

                    if ((result & process_result::HANDLED_TRUE) &&
                        (active_state_ids != m_active_state_ids))
                    {
                        // The active state configuration has changed.
                        // Start from the beginning, we might be able
                        // to process events that stayed in the queue before.
                        active_state_ids = m_active_state_ids;
                        it = deferred_events.queue.begin();
                    }
                }
            } while (it != deferred_events.queue.end());
        }
    }

    // Handling of completion transitions.
    template <typename Transition>
    using is_completion_transition =
        has_completion_event<typename Transition::transition_event>;

    void try_process_completion_event(EventSource source, bool handled)
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
                process_event_internal(
                    compile_policy_impl::normalize_event(completion_event{}),
                    source | EventSource::EVENT_SOURCE_DIRECT);
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

    // Main function used internally to make transitions.
    // Can only be called for internally (for example in an action method) generated events.
    template <class Event>
    process_result process_event_internal(
        Event const& event,
        EventSource source = EventSource::EVENT_SOURCE_DEFAULT)
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

        // If we have an event queue and are already processing events,
        // enqueue it for later processing.
        if constexpr (events_queue_member::value)
        {
            if (m_event_processing)
            {
                enqueue_event(event);
                return process_result::HANDLED_TRUE;
            }
        }

        // If deferred events are configured and the event could be deferred
        // in the active state configuration, then conditionally defer it for later processing.
        if constexpr (has_deferring_states::value &&
                      compile_policy_impl::template has_deferred_event<
                          deferring_states, Event>::value)
        {
            if (compile_policy_impl::is_event_deferred(*this, event))
            {
                compile_policy_impl::defer_event(*this, event);
                return process_result::HANDLED_DEFERRED;
            }
        }

        // Process the event.
        m_event_processing = true;
        process_result result;
        const bool is_direct_call = source & EventSource::EVENT_SOURCE_DIRECT;
        if constexpr (has_no_exception_thrown<front_end_t>::value)
        {
            result = do_process_event(event, is_direct_call);
        }
        else
        {
            // when compiling without exception support there is no formal parameter "e" in the catch handler.
            // Declaring a local variable here does not hurt and will be "used" to make the code in the handler
            // compilable although the code will never be executed.
            std::exception e;
            BOOST_TRY
            {
                result = do_process_event(event, is_direct_call);
            }
            BOOST_CATCH (std::exception& e)
            {
                // give a chance to the concrete state machine to handle
                this->exception_caught(event, get_fsm_argument(), e);
                result = process_result::HANDLED_FALSE;
            }
            BOOST_CATCH_END
        }

        // at this point we allow the next transition be executed without enqueing
        // so that completion events and deferred events execute now (if any)
        m_event_processing = false;

        // Process completion transitions BEFORE any other event in the
        // pool (UML Standard 2.3 15.3.14)
        try_process_completion_event(source, (result & process_result::HANDLED_TRUE));

        // After handling, take care of the queued and deferred events.
        // Default:
        // Handle deferred events queue with higher prio than events queue.
        if constexpr (!has_event_queue_before_deferred_queue<front_end_t>::value)
        {
            if (!(EventSource::EVENT_SOURCE_DEFERRED & source))
            {
                try_process_deferred_events();

                // Handle any new events generated into the queue, but only if
                // we're not already processing from the message queue.
                if (!(EventSource::EVENT_SOURCE_MSG_QUEUE & source))
                {
                    try_process_queued_events();
                }
            }
        }
        // Non-default:
        // Handle events queue with higher prio than deferred events queue.
        else
        {
            if (!(EventSource::EVENT_SOURCE_MSG_QUEUE & source))
            {
                try_process_queued_events();
                if (!(EventSource::EVENT_SOURCE_DEFERRED & source))
                {
                    try_process_deferred_events();
                }
            }
        }

        return result;
    }

    // minimum event processing without exceptions, queues, etc.
    template<class Event>
    process_result do_process_event(Event const& event, bool is_direct_call)
    {
        if constexpr (deferred_events_member::value)
        {
            if (is_direct_call)
            {
                get_deferred_events().cur_seq_cnt += 1;
            }
        }

        using dispatch_table =
            typename compile_policy_impl::template dispatch_table<derived_t,
                                                                  Event>;
        process_result result = process_result::HANDLED_FALSE;
        // Dispatch the event to every region.
        for (int region_id = 0; region_id < nr_regions; region_id++)
        {
            result |= dispatch_table::dispatch(*static_cast<derived_t*>(this), m_active_state_ids[region_id], event);
        }
        // Dispatch the event to the SM-internal table if it hasn't been consumed yet.
        if (!(result & handled_or_deferred))
        {
            result |= dispatch_table::internal_dispatch(*static_cast<derived_t*>(this), event);
        }

        // if the event has not been handled and we have orthogonal zones, then
        // generate an error on every active state
        // for state machine states contained in other state machines, do not handle
        // but let the containing sm handle the error, unless the event was generated in this fsm
        // (by calling process_event on this fsm object, is_direct_call == true)
        // completion events do not produce an error
        if ((!is_contained() || is_direct_call) && !result && !compile_policy_impl::is_completion_event(event))
        {
            for (const auto state_id: m_active_state_ids)
            {
                this->no_transition(event, get_fsm_argument(), state_id);
            }
        }
        return result;
    }

private:
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

        // Give a chance to handle a completion transition first.
        try_process_completion_event(EventSource::EVENT_SOURCE_DEFAULT, true);

        // Default:
        // Handle deferred events queue with higher prio than events queue.
        if constexpr (!has_event_queue_before_deferred_queue<front_end_t>::value)
        {
            try_process_deferred_events();
            try_process_queued_events();
        }
        // Non-default:
        // Handle events queue with higher prio than deferred events queue.
        else
        {
            try_process_queued_events();
            try_process_deferred_events();
        }        
    }

    template <class Event, class Fsm>
    void on_entry(Event const& event, Fsm& fsm)
    {
        preprocess_entry(event, fsm);

        // First set all active state ids...
        m_active_state_ids = m_history.on_entry(event);

        // ... then execute each state entry.
        if constexpr (std::is_same_v<typename front_end_t::history,
                                     front::no_history>)
        {
            mp11::mp_for_each<initial_state_identities>(
                [this, &event](auto state_identity)
                {
                    using State = typename decltype(state_identity)::type;
                    auto& state = this->get_state<State>();
                    state.on_entry(event, get_fsm_argument());
                });
        }
        else
        {
            visit<visit_mode::active_non_recursive>(
                [this, &event](auto& state)
                {
                    state.on_entry(event, get_fsm_argument());
                });
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
        if constexpr (all_regions_defined)
        {
            mp11::mp_for_each<state_identities>(
                [this, &event](auto state_identity)
                {
                    using State = typename decltype(state_identity)::type;
                    auto& state = this->get_state<State>();
                    state.on_entry(event, get_fsm_argument());
                });
        }
        else
        {
            visit<visit_mode::active_non_recursive>(
                [this, &event](auto& state)
                {
                    state.on_entry(event, get_fsm_argument());
                });
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

    template <class Event,class Fsm>
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
        // History decides what happens with deferred events.
        if (!m_history.process_deferred_events(event))
        {
            if constexpr (deferred_events_member::value)
            {
                get_deferred_events_queue().clear();
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
