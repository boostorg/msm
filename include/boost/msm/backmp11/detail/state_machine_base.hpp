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

#ifndef BOOST_MSM_BACKMP11_DETAIL_STATE_MACHINE_BASE_HPP
#define BOOST_MSM_BACKMP11_DETAIL_STATE_MACHINE_BASE_HPP

#include <array>
#include <cstdint>
#include <utility>

#include <boost/assert.hpp>
#include <boost/mp11.hpp>

#include <boost/msm/row_tags.hpp>
#include <boost/msm/backmp11/detail/favor_runtime_speed.hpp>
#include <boost/msm/backmp11/detail/history_impl.hpp>
#include <boost/msm/backmp11/detail/state_machine_core.hpp>
#include <boost/msm/backmp11/detail/state_visitor.hpp>
#include <boost/msm/backmp11/detail/transition_table.hpp>

namespace boost::msm::backmp11::detail
{

// Wrapper to invoke a reflect free function
// (required for ADL).
struct invoke_reflect_free
{
    template <typename State, typename Visitor>
    void operator()(State&& state, Visitor&& visitor)
    {
        reflect(std::forward<State>(state), std::forward<Visitor>(visitor));
    }
};

template <class FrontEnd, class Config, class Derived>
class state_machine_base
    : public state_machine_core<
          Config, get_nesting_role<Config, Derived>()>,
      public FrontEnd
{
    using state_machine_core =
        detail::state_machine_core<Config, get_nesting_role<Config, Derived>()>;
    using event_pool_processor = typename state_machine_core::event_pool_processor;

    static_assert(
        is_composite<FrontEnd>::value,
        "FrontEnd must be a composite state");

  public:
    using config_t = typename state_machine_core::config_t;
    using root_sm_t = typename state_machine_core::root_sm_t;
    using context_t = typename state_machine_core::context_t;
    using front_end_t = FrontEnd;
    using derived_t = Derived;
    using starting = backmp11::starting;
    using stopping = backmp11::stopping;

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
        using forward_fn_t = void (*)(void* /*root_sm*/, const void* /*event*/);

        template <typename RootSm>
        void init()
        {
            m_forward_fn = &call_enqueue_event<RootSm, event>;
        }

        // forward event to the root sm.
        template <class ForwardEvent>
        void forward_event(void* root_sm, const ForwardEvent& forward_event)
        {
            static_assert(
                std::is_convertible_v<ForwardEvent, event>,
                "ForwardEvent must be convertible to exit pseudostate's event");
            // Call if handler set.
            // If not, this state is simply a terminate state.
            if (m_forward_fn)
            {
                m_forward_fn(root_sm, &forward_event);
            }
        }

      private:
        template <typename RootSm, typename Event>
        static void call_enqueue_event(void* root_sm, const void* event)
        {
            static_cast<RootSm*>(root_sm)->enqueue_event(
                *static_cast<const Event*>(event));
        }

        forward_fn_t m_forward_fn{};
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
        static constexpr auto nr_regions = mp11::mp_size<initial_states>::value;

        using state_set = generate_state_set<state_machine_base>;
        using state_map = generate_state_map<state_set>;
        template <typename State>
        using get_state_id =
            detail::get_state_id<state_map, State>;

        using submachines = mp11::mp_copy_if<state_set, is_composite>;
    };

    using states_t = mp11::mp_rename<typename internal::state_set, std::tuple>;

  private:
    using state_set = typename internal::state_set;
    using state_map = typename internal::state_map;
    static constexpr auto nr_regions = internal::nr_regions;
    using active_state_ids_t = std::array<uint16_t, nr_regions>;
    using initial_state_ids =
        mp11::mp_transform<internal::template get_state_id,
                           typename internal::initial_states>;
    using compile_policy_impl = detail::compile_policy_impl<typename config_t::compile_policy>;

    template <class, class, class>
    friend class state_machine_base;

    template <typename, typename>
    friend class history_impl;

    template <typename StateMachine>
    friend struct transition_table_impl;

    template <typename Policy, typename>
    friend struct detail::compile_policy_impl;

    template <typename, bool, template <typename> typename...>
    friend class state_visitor_base_impl;
    template <typename, typename, template <typename> typename...>
    friend class event_deferral_visitor;

    template <typename Event>
    friend class deferred_event;

    template<typename T0, typename T1, typename T2, typename F>
    friend void reflect(state_machine_base<T0, T1, T2>&, F&&);
    template<typename T0, typename T1, typename T2, typename F>
    friend void reflect(const state_machine_base<T0, T1, T2>&, F&&);
    
    using history_impl = detail::history_impl<typename front_end_t::history,
                                              initial_state_ids>;

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
            *this->m_root_sm = this;
            using visitor_t = init_state_visitor<derived_t>;
            visitor_t visitor{self()};
            visit_if<visit_mode::all_recursive, 
                     visitor_t::template predicate>(self(), visitor);
        }
    }

    // Construct with a context and
    // forward further constructor arguments to the front-end.
    template <bool C = state_machine_core::has_context_member,
              typename = std::enable_if_t<C>,
              typename... Args>
    state_machine_base(context_t& context, Args&&... args)
        : state_machine_base(std::forward<Args>(args)...)
    {
        this->m_context = &context;
    }

    // Copy constructor.
    state_machine_base(state_machine_base const& rhs) : state_machine_base()
    {
        *this = rhs;
    }

    // Copy assignment operator.
    state_machine_base& operator=(state_machine_base const& rhs) = default;

    // Move constructor.
    state_machine_base(state_machine_base&& rhs) : state_machine_base()
    {
        *this = std::move(rhs);
    }

    // Move assignment operator.
    state_machine_base& operator=(state_machine_base&& rhs) = default;

    // Start the state machine (calls entry of the initial state(s)).
    void start()
    {
        start(starting{});
    }

    // Start the state machine
    // (calls entry of the initial state(s) with initial_event).
    template <class Event>
    void start(Event const& initial_event)
    {
        // Assert for a case where root sm was not set up correctly
        // after construction.
        if constexpr (!std::is_same_v<typename Config::root_sm, no_root_sm>)
        {
            BOOST_ASSERT_MSG(&(this->get_root_sm()),
            "Root sm must be passed as Derived and configured as root_sm");
        }
        if (this->m_machine_state == machine_state::stopped)
        {
            on_entry(initial_event, get_fsm_argument());
        }
    }

    // Stop the state machine (calls exit of the current state(s)).
    void stop()
    {
        stop(stopping{});
    }

    // Stop the state machine
    // (calls exit of the current state(s) with final_event).
    template <class Event>
    void stop(Event const& final_event)
    {
        if (this->m_machine_state != machine_state::stopped)
        {
            on_exit(final_event, get_fsm_argument());
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

    // Enqueues an event in the event pool for later processing.
    // If the state machine is already processing, the event will be processed
    // after the current event completes.
    template <class Event,
              bool C = state_machine_core::has_event_pool,
              typename = std::enable_if_t<C>>
    void enqueue_event(Event const& event)
    {
        compile_policy_impl::defer_event(
            *this, compile_policy_impl::normalize_event(event), false);
    }

    // Puts the event into the event pool for later processing.
    // If the deferral takes place while the state machine is processing,
    // the event will be evaluated for dispatch from the next processing cycle.
    template <
        class Event,
        bool C = state_machine_core::has_event_pool,
        typename = std::enable_if_t<C>>
    void defer_event(Event const& event)
    {
        compile_policy_impl::defer_event(
            *this, compile_policy_impl::normalize_event(event),
            this->m_machine_state == machine_state::processing);
    }

    // Getter that returns the currently active state ids of the FSM.
    const active_state_ids_t& get_active_state_ids() const
    {
        return m_active_state_ids;
    }

    // Return the id of a state in the sm.
    template<typename State>
    static constexpr size_t get_state_id(const State&)
    {
        static_assert(
            mp11::mp_map_contains<state_map, State>::value,
            "The state must be contained in the state machine");
        return detail::get_state_id<state_map, State>::value;
    }
    // Return the id of a state in the sm.
    template<typename State>
    static constexpr size_t get_state_id()
    {
        static_assert(
            mp11::mp_map_contains<state_map, State>::value,
            "The state must be contained in the state machine");
        return detail::get_state_id<state_map, State>::value;
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
        visit_if<Mode>(self(), std::forward<Visitor>(visitor));
    }

    // Visit the states.
    // How to traverse is selected with visit_mode.
    template <visit_mode Mode, typename Visitor>
    void visit(Visitor&& visitor) const
    {
        visit_if<Mode>(self(), std::forward<Visitor>(visitor));
    }

    // Check whether a state is currently active.
    template <typename State>
    bool is_state_active() const
    {
        using visitor_t = is_state_active_visitor<State>;
        visitor_t visitor;
        visit_if<visit_mode::active_recursive,
                 visitor_t::template predicate>(self(), visitor);
        return visitor.result();
    }

    // Check if a flag is active, using the BinaryOp as folding function.
    template <typename Flag, typename BinaryOp = flag_or>
    bool is_flag_active() const
    {
        using visitor_t = is_flag_active_visitor<Flag, BinaryOp>;
        visitor_t visitor;
        visit_if<visit_mode::active_recursive,
                 visitor_t::template predicate>(self(), visitor);
        return visitor.result();
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

    const fsm_parameter_t& get_fsm_argument() const
    {
        if constexpr (std::is_same_v<typename config_t::fsm_parameter,
                                     local_transition_owner>)
        {
            return self();
        }
        else
        {
            return this->get_root_sm();
        }
    }

    fsm_parameter_t& get_fsm_argument()
    {
        return const_cast<fsm_parameter_t&>
            (static_cast<const state_machine_base&>(*this).get_fsm_argument());
    }

    template <typename Event>
    bool is_event_deferred(const Event& event) const
    {
        return compile_policy_impl::is_event_deferred(self(), event);
    }

    // Repetition of the front-end's method definition
    // required due to above signature.
    template <typename Event, typename Fsm>
    bool is_event_deferred(const Event& event, Fsm& fsm) const
    {
        return static_cast<const front_end_t*>(this)->is_event_deferred(event,
                                                                        fsm);
    }

    // Checks if an event is an end interrupt event.
    template <typename Event>
    bool is_end_interrupt_event(const Event& event) const
    {
        return compile_policy_impl::is_end_interrupt_event(*this, event);
    }

    // Main function used internally to process events.
    template <class Event>
    process_result process_event_internal(Event const& event, process_info info)
    {
        if (this->m_machine_state != machine_state::idle)
        {
            return process_result::HANDLED_FALSE;
        }

        // If the state machine has terminate or interrupt flags, check them.
        if constexpr (mp11::mp_any_of<state_set, is_state_blocking>::value)
        {
            // If the state machine is terminated, discard the event.
            if (is_flag_active<TerminateFlag>())
            {
                return process_result::HANDLED_TRUE;
            }

            // If the state machine is interrupted, discard the event
            // unless it is the end interrupt event.
            if (is_flag_active<InterruptedFlag>() &&
                !is_end_interrupt_event(event))
            {
                return process_result::HANDLED_TRUE;
            }
        }

        if constexpr (state_machine_core::has_event_pool)
        {
            if (info != process_info::event_pool)
            {
                // If the event is deferred in the
                // active state configuration, process it later.
                // Skip the deferral check in submachine calls, since the
                // parent has already checked and dispatched the event.
                if (info != process_info::submachine_call &&
                    compile_policy_impl::try_defer_event(self(), event))
                {
                    return process_result::HANDLED_DEFERRED;
                }

                // Ensure we consider an event
                // that was action-deferred in the last sequence.
                this->get_event_pool().cur_seq_cnt += 1;
            }
        }

        // Process the event.
        process_result result;
        {
            process_guard guard{this->m_machine_state};
            result = do_process_event(event, info);
        }

        // After handling, look if we have more to process in the event pool
        // (but only if we're not already processing from it).
        if constexpr (state_machine_core::has_event_pool)
        {
            if (info != process_info::event_pool)
            {
                this->process_event_pool();
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
        for (uint8_t region_id = 0; region_id < nr_regions; region_id++)
        {
            result |= dispatch_table::dispatch(self(), region_id, event);
        }
        // Dispatch the event to the SM-internal table if it hasn't been
        // consumed yet.
        if (!(result & handled_true_or_deferred))
        {
            result |= dispatch_table::internal_dispatch(self(), event);
        }

        // If the event has not been handled and we have orthogonal zones,
        // then generate an error on every active state. For events coming
        // from upper machines, do not handle but let the upper sm handle
        // the error.
        if (!result && !(info == process_info::submachine_call))
        {
            for (const auto state_id : m_active_state_ids)
            {
                this->no_transition(event, get_fsm_argument(), state_id);
            }
        }
        return result;
    }

    // MSCV Bug:
    // Compile error if this class is named completion_event.
    template <typename State>
    class completion_event_occurrence : public event_occurrence
    {
        // Merge each list of transitions into a chain if needed.
        template <typename Transitions>
        struct merge_transitions_impl;
        template <typename Transition>
        struct merge_transitions_impl<mp11::mp_list<Transition>>
        {
            using type = Transition;
        };
        template <typename... Transitions>
        struct merge_transitions_impl<mp11::mp_list<Transitions...>>
        {
            using list = mp11::mp_list<Transitions...>;
            using completion_event =
                typename mp11::mp_first<list>::transition_event;
            using type =
                transition_chain<derived_t, State, list, completion_event>;
        };
        template <typename Transitions>
        using merge_transitions =
            typename merge_transitions_impl<Transitions>::type;
        using completion_transitions =
            detail::completion_transitions<derived_t, State>;
        using completion_transition = merge_transitions<completion_transitions>;

      public:
        completion_event_occurrence(uint8_t region_id)
            : event_occurrence(&try_process), m_region_id(region_id)
        {
        }

        static std::optional<process_result> try_process(
            event_occurrence& self, void* processor,
            uint16_t /*seq_cnt*/)
        {
            return static_cast<completion_event_occurrence&>(self)
                .try_process_impl(static_cast<derived_t&>(
                    *static_cast<event_pool_processor*>(processor)));
        }

      private:
        std::optional<process_result> try_process_impl(derived_t& sm)
        {
            mark_for_deletion();
            return sm.template process_completion_transition<
                completion_transition>(m_region_id);
        }

        uint8_t m_region_id;
    };

    template <typename Transition>
    process_result process_completion_transition(uint8_t region_id)
    {
        // If the state machine has terminate or interrupt flags, check them.
        if constexpr (mp11::mp_any_of<state_set, is_state_blocking>::value)
        {
            // If the state machine is interrupted or terminated, do not handle any event.
            if (is_flag_active<TerminateFlag>() ||
                is_flag_active<InterruptedFlag>())
            {
                return process_result::HANDLED_TRUE;
            }
        }

        // Process the event.
        using completion_event = typename Transition::transition_event;
        {
            process_guard guard{this->m_machine_state};
            return Transition::execute(self(), region_id, completion_event{});
        }
    }

    template <class Event>
    void do_defer_event(const Event& event, bool next_rtc_seq)
    {
        auto& event_pool = this->get_event_pool();
        const uint16_t seq_cnt = next_rtc_seq ? event_pool.cur_seq_cnt
                                              : event_pool.cur_seq_cnt - 1;
        event_pool.events.push_back(event_pool_processor::processable_event::make(
            deferred_event<Event>{self(), event, seq_cnt}));
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
            m_self.template on_state_entry_completed<State>(m_region_id++);
        }

      private:
        derived_t& m_self;
        const Event& m_event;
        uint8_t m_region_id{};
    };


    template <class Event, class Fsm>
    void on_entry(Event const& event, Fsm& fsm)
    {
        {
            process_guard guard{this->m_machine_state};

            // First set all active state ids...
            history_impl::on_entry(self(), event);
            
            // ... then execute each state entry.
            static_cast<front_end_t*>(this)->on_entry(event, fsm);
            state_entry_visitor<Event> visitor{self(), event};
            history_impl::on_entry(self(), visitor);
        }

        // After handling, look if we have more to process in the event pool.
        if constexpr (state_machine_core::has_event_pool)
        {
            this->process_event_pool();
        }
    }

    template <class TargetStates, class Event, class Fsm>
    void on_explicit_entry(Event const& event, Fsm& fsm)
    {
        {
            process_guard guard{this->m_machine_state};

            // First set all active state ids...
            using state_identities =
                mp11::mp_transform<mp11::mp_identity, TargetStates>;
            static constexpr bool all_regions_defined =
                mp11::mp_size<state_identities>::value == nr_regions;
            if constexpr (!all_regions_defined)
            {
                history_impl::on_entry(self(), event);
            }
            mp11::mp_for_each<state_identities>(
                [this](auto state_identity)
                {
                    using State = typename decltype(state_identity)::type;
                    static constexpr uint8_t region_id = State::zone_index;
                    static_assert(region_id < nr_regions);
                    m_active_state_ids[region_id] = get_state_id<State>();
                });
            
            // ... then execute each state entry.
            static_cast<front_end_t*>(this)->on_entry(event, fsm);
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
        }

        // After handling, look if we have more to process in the event pool.
        if constexpr (state_machine_core::has_event_pool)
        {
            this->process_event_pool();
        }
    }

    template <class TargetStates, class Event, class Fsm>
    void on_pseudo_entry(Event const& event, Fsm& fsm)
    {
        on_explicit_entry<TargetStates>(event, fsm);

        // Execute the second part of the compound transition.
        process_event(event);
    }

    template <typename State>
    void on_state_entry_completed(uint8_t region_id)
    {
        // Exclude composite states from completion transitions,
        // these should fire when all their regions reach a final state
        // (and final states do not exist yet).
        if constexpr(
            !is_composite<State>::value &&
            has_completion_transitions<derived_t, State>::value)
        {
            auto& event_pool = this->get_event_pool();
            // Process completion transitions BEFORE any other event in the
            // pool (UML Standard 2.3 15.3.14).
            event_pool.events.push_front(
                event_pool_processor::processable_event::make(
                    completion_event_occurrence<State>{region_id}));
        }
    }

    template <class Event, class Fsm>
    void on_exit(Event const& event, Fsm& fsm)
    {
        {
            process_guard guard{this->m_machine_state};

            // First exit the substates...
            visit<visit_mode::active_non_recursive>(
                [this, &event](auto& state)
                {
                    state.on_exit(event, get_fsm_argument());
                });
            // ... then call our own exit.
            (static_cast<front_end_t*>(this))->on_exit(event, fsm);
        }
        this->m_machine_state = machine_state::stopped;
    }

    derived_t& self()
    {
        return *static_cast<derived_t*>(this);
    }

    const derived_t& self() const
    {
        return *static_cast<const derived_t*>(this);
    }

    template <typename State, typename F, typename = void>
    struct has_reflect_member : std::false_type {};
    template <typename State, typename Visitor>
    struct has_reflect_member<State, Visitor,
        std::void_t<decltype(std::declval<State&>().reflect(std::declval<Visitor&&>()))>>
        : std::true_type {};

    template <typename State, typename Visitor, typename = void>
    struct has_reflect_free : std::false_type {};
    template <typename State, typename Visitor>
    struct has_reflect_free<State, Visitor,
        std::void_t<decltype(reflect(std::declval<State&>(), std::declval<Visitor&&>()))>>
        : std::true_type {};

    template <typename Self, typename Visitor>
    static void reflect_impl(Self& self, Visitor&& visitor)
    {
        using FrontEndRef = mp11::mp_if_c<
            std::is_const_v<Self>,
            const FrontEnd&,
            FrontEnd&>;
        
        auto& front_end = static_cast<FrontEndRef>(self);
        if constexpr (has_reflect_member<FrontEnd, Visitor>::value)
        {
            visitor.visit_front_end(front_end, [&front_end, &visitor]() {
                front_end.reflect(std::forward<Visitor>(visitor));
            });
        }
        else if constexpr (has_reflect_free<FrontEnd, Visitor>::value)
        {
            visitor.visit_front_end(front_end, [&front_end, &visitor]() {
                invoke_reflect_free{}(front_end, std::forward<Visitor>(visitor));
            });
        }
        else
        {
            visitor.visit_front_end(front_end);
        }
        // root_sm, event pool and context cannot be serialized.
        mp11::tuple_for_each(self.m_states,
        [&visitor](auto& state)
        {
            using State = std::decay_t<decltype(state)>;

            if constexpr (has_reflect_member<State, Visitor>::value ||
                          has_state_machine_tag<State>::value)
            {
                visitor.visit_state(get_state_id<State>(), state, [&state, &visitor]() {
                    state.reflect(std::forward<Visitor>(visitor));
                });
            }
            else if constexpr (has_reflect_free<State, Visitor>::value)
            {
                visitor.visit_state(get_state_id<State>(), state, [&state, &visitor]() {
                    invoke_reflect_free{}(state, std::forward<Visitor>(visitor));
                });
            }
            else
            {
                visitor.visit_state(get_state_id<State>(), state);
            }
        });
        visitor.visit_member("active_state_ids", self.m_active_state_ids);
        visitor.visit_member("machine_state", self.m_machine_state);
    }

    template <typename Visitor>
    void reflect(Visitor&& visitor)
    {
        reflect_impl(*this, std::forward<Visitor>(visitor));
    }

    template <typename Visitor>
    void reflect(Visitor&& visitor) const
    {
        reflect_impl(*this, std::forward<Visitor>(visitor));
    }

    states_t               m_states{};
    active_state_ids_t     m_active_state_ids{value_array<initial_state_ids>};
};

std::false_type is_state_machine(...);

template <typename A0, typename A1, typename A2>
std::true_type is_state_machine(state_machine_base<A0, A1, A2>*);

template <typename T>
constexpr bool is_state_machine_v =
    decltype(is_state_machine(std::declval<T*>()))::value;

template <typename FrontEnd, typename Config, typename Derived,
          typename Visitor>
void reflect(state_machine_base<FrontEnd, Config, Derived>& sm,
             Visitor&& visitor)
{
    sm.reflect(std::forward<Visitor>(visitor));
}

template <typename FrontEnd, typename Config, typename Derived,
          typename Visitor>
void reflect(const state_machine_base<FrontEnd, Config, Derived>& sm,
             Visitor&& visitor)
{
    sm.reflect(std::forward<Visitor>(visitor));
}

} // boost::msm::backmp11::detail

#endif // BOOST_MSM_BACKMP11_DETAIL_STATE_MACHINE_BASE_HPP
