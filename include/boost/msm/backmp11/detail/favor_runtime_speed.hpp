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

#ifndef BOOST_MSM_BACKMP11_DETAIL_FAVOR_RUNTIME_SPEED_H
#define BOOST_MSM_BACKMP11_DETAIL_FAVOR_RUNTIME_SPEED_H

#include <boost/msm/backmp11/detail/metafunctions.hpp>
#include <boost/msm/backmp11/detail/dispatch_table.hpp>
#include <boost/msm/backmp11/detail/transition_table.hpp>
#include <boost/msm/backmp11/event_traits.hpp>

namespace boost { namespace msm { namespace backmp11
{

struct favor_runtime_speed {};

namespace detail
{

template <typename Policy>
struct compile_policy_impl;
template <>
struct compile_policy_impl<favor_runtime_speed>
{
    // Bitmask for process result checks.
    static constexpr process_result handled_or_deferred =
        process_result::HANDLED_TRUE | process_result::HANDLED_DEFERRED;

    template <typename Event>
    static constexpr bool is_completion_event(const Event&)
    {
        return has_completion_event<Event>::value;
    }

    template <typename StateMachine, typename Event>
    static bool is_end_interrupt_event(StateMachine& sm, const Event&)
    {
        return sm.template is_flag_active<EndInterruptFlag<Event>>();
    }

    template <typename Event>
    constexpr static const Event& normalize_event(const Event& event)
    {
        return event;
    }

    template <typename StateMachine, typename Event>
    class deferred_event_impl : public deferred_event
    {
      public:
        deferred_event_impl(StateMachine& sm, const Event& event, size_t seq_cnt)
            : deferred_event(seq_cnt), m_sm(sm), m_event(event)
        {
        }

        process_result process() override
        {
            return m_sm.process_event_internal(
                m_event,
                EventSource::EVENT_SOURCE_DEFERRED);
        }

        bool is_deferred() const override
        {
            return is_event_deferred(m_sm, m_event);
        }

      private:
        StateMachine& m_sm;
        Event m_event;
    };

    template <typename StateOrStates, typename Event>
    using has_deferred_event = has_deferred_event<StateOrStates, Event>;

    template <typename StateMachine, typename Event>
    class is_event_deferred_helper
    {
      public:
        static bool execute(const StateMachine& sm)
        {
            bool result = false;
            auto visitor = [&result](const auto& /*state*/)
            {
                result = true;
            };
            // Apply a pre-filter with the 'has_deferred_events' predicate,
            // since this subset needs to be instantiated only once for the SM.
            sm.template visit_if<visit_mode::active_non_recursive,
                                 has_deferred_events, has_deferred_event>(visitor);
            return result;
        }

      private:
        using deferring_states = typename StateMachine::deferring_states;
        template <typename State>
        using has_deferred_event = has_deferred_event<State, Event>;
    };

    template <typename StateMachine, typename Event>
    static bool is_event_deferred(const StateMachine& sm, const Event&)
    {
        using helper = is_event_deferred_helper<StateMachine, Event>;
        return helper::execute(sm);
    }

    template <typename StateMachine, typename Event>
    static void defer_event(StateMachine& sm, Event const& event)
    {
        if constexpr (is_kleene_event<Event>::value)
        {
            using event_set = generate_event_set<
                typename StateMachine::front_end_t::transition_table>;
            bool found =
                mp_for_each_until<mp11::mp_transform<mp11::mp_identity, event_set>>(
                    [&sm, &event](auto event_identity)
                    {
                        using KnownEvent = typename decltype(event_identity)::type;
                        if (event.type() == typeid(KnownEvent))
                        {
                            do_defer_event(sm, *any_cast<KnownEvent>(&event));
                            return true;
                        }
                        return false;
                    }
            );
            if (!found)
            {
                for (const auto state_id : sm.get_active_state_ids())
                {
                    sm.no_transition(event, sm.get_fsm_argument(), state_id);
                }
            }
        }
        else
        {
            do_defer_event(sm, event);
        }
    }

    template <typename StateMachine, class Event>
    static void do_defer_event(StateMachine& sm, const Event& event)
    {
        auto& deferred_events = sm.get_deferred_events();
        deferred_events.queue.push_back(basic_unique_ptr<deferred_event>{
            new deferred_event_impl<StateMachine, Event>(
                sm, event, deferred_events.cur_seq_cnt)});
    }

    // Generates a singleton runtime lookup table that maps current state
    // to a function that makes the SM take its transition on the given
    // Event type.
    template <class StateMachine, typename Event>
    class dispatch_table
    {
      public:
        // Dispatch an event.
        static process_result dispatch(StateMachine& sm, int& state_id, const Event& event)
        {
            if constexpr (has_transitions::value ||
                          has_forward_transitions::value)
            {
                const dispatch_table_impl& self = dispatch_table_impl::instance();
                return self.dispatch(sm, state_id, event);
            }
            return process_result::HANDLED_FALSE;
        }

        // Dispatch an event to the SM's internal table.
        static process_result internal_dispatch(StateMachine& sm, const Event& event)
        {
            if constexpr (has_internal_transitions::value)
            {
                return internal_dispatch_impl::transition::execute(sm, event);
            }
            return process_result::HANDLED_FALSE;
        }

      private:
        using cell_t = process_result (*)(StateMachine&, int&, Event const&);

        // All dispatch tables are friend with each other to check recursively
        // whether forward transitions are required.
        template <class, typename>
        friend class dispatch_table;

        // Compute the maximum state value in the sm so we know how big
        // to make the tables.
        using state_set = typename StateMachine::state_set;
        static constexpr int max_state = mp11::mp_size<state_set>::value;

        // Filter the transition tables by event.
        template <typename Transition,
                  bool IsKleeneEvent = is_kleene_event<
                      typename Transition::transition_event>::value>
        struct transition_event_predicate_impl
        {
            using type = std::is_base_of<typename Transition::transition_event, Event>;
        };
        template <typename Transition>
        struct transition_event_predicate_impl<Transition, /*IsKleeneEvent=*/true>
        {
            using type = mp11::mp_true;
        };
        template <typename Transition>
        using transition_event_predicate = 
            typename transition_event_predicate_impl<Transition>::type;
        
        using filtered_internal_transition_table = mp11::mp_copy_if<
                internal_transition_table<StateMachine>,
                transition_event_predicate>;
        using has_internal_transitions =
            mp11::mp_not<mp11::mp_empty<filtered_internal_transition_table>>;

        using filtered_transition_table = mp11::mp_copy_if<
                transition_table<StateMachine>,
                transition_event_predicate>;
        using has_transitions = mp11::mp_not<mp11::mp_empty<filtered_transition_table>>;

        // All submachines that could process the event or need to
        // forward it to sub-submachines require a pseudo-transition
        // for forwarding.
        // This function recursively walks through submachines and
        // uses the same metafunctions for checking which are 
        // anyways needed for dispatch table creation.
        template <typename Submachine>
        struct needs_forward_transition_impl
        {
            template <typename Subsubmachine>
            using needs_forward_transition =
                typename needs_forward_transition_impl<Subsubmachine>::type;

            using type = mp11::mp_or<
                typename dispatch_table<Submachine, Event>::has_transitions,
                typename dispatch_table<Submachine, Event>::has_internal_transitions,
                mp11::mp_any_of<
                    typename Submachine::internal::submachines,
                    needs_forward_transition>>;
        };
        template <typename Submachine>
        using needs_forward_transition =
            typename needs_forward_transition_impl<Submachine>::type;

        using submachines = mp11::mp_copy_if<state_set, is_composite>;
        using submachines_with_forward_transitions =
            mp11::mp_copy_if<submachines, needs_forward_transition>;
        using has_forward_transitions =
            mp11::mp_not<mp11::mp_empty<submachines_with_forward_transitions>>;

        class dispatch_table_impl
        {
          public:
            static const dispatch_table_impl& instance() {
                static dispatch_table_impl table;
                return table;
            }

            process_result dispatch(
                StateMachine& sm, int& state_id, const Event& event) const
            {
                const cell_t& cell = m_cells[state_id];
                if (cell)
                {
                    return cell(sm, state_id, event);
                }
                return process_result::HANDLED_FALSE;
            }

          private:
            dispatch_table_impl()
            {
                using initial_map_items = mp11::mp_transform<
                    initial_map_item,
                    submachines_with_forward_transitions>;
                using filtered_transitions_by_state_map = mp11::mp_fold<
                    filtered_transition_table,
                    initial_map_items,
                    map_updater>;
                using merged_transitions = mp11::mp_transform<
                    merge_transitions,
                    filtered_transitions_by_state_map>;
                using init_cell_constants = mp11::mp_transform<
                    get_init_cell_constant,
                    merged_transitions>;
                dispatch_table_initializer::execute(
                    reinterpret_cast<generic_cell*>(m_cells),
                    reinterpret_cast<const generic_init_cell_value*>(
                        value_array<init_cell_constants>),
                    mp11::mp_size<init_cell_constants>::value);
            }

            template <typename Transition>
            static process_result convert_event_and_execute(StateMachine& sm,
                                                            int& state_id,
                                                            Event const& evt)
            {
                typename Transition::transition_event kleene_event{evt};
                return Transition::execute(sm, state_id, kleene_event);
            }

            template <size_t index, cell_t cell>
            using init_cell_constant = init_cell_constant<index, cell_t, cell>;

            // Build a map with key=state/value=[matching_transitions]
            // from the filtered transition table.
            template <typename M, typename Key, typename Value>
            using push_map_value = mp11::mp_push_back<
                mp11::mp_second<mp11::mp_map_find<M, Key>>,
                Value>;
            template <
                typename Map,
                typename Transition,
                bool FirstEntry = !mp11::mp_map_contains<
                    Map, typename Transition::current_state_type>::value>
            struct map_updater_impl;
            template <typename Map, typename Transition>
            struct map_updater_impl<Map, Transition, /*FirstEntry=*/false>
            {
                using type = mp11::mp_map_replace<
                    Map,
                    mp11::mp_list<
                        typename Transition::current_state_type,
                        push_map_value<
                            Map,
                            typename Transition::current_state_type,
                            Transition>>>;
            };
            template <typename Map, typename Transition>
            struct map_updater_impl<Map, Transition, /*FirstEntry=*/true>
            {
                using type = mp11::mp_map_replace<
                    Map,
                    mp11::mp_list<
                        typename Transition::current_state_type,
                        mp11::mp_list<Transition>>>;
            };
            template <typename Map, typename Transition>
            using map_updater = typename map_updater_impl<Map, Transition>::type;

          private:
            // Pseudo-transition used to forward event processing to a submachine.
            template <typename Submachine>
            struct forward_transition
            {
                using current_state_type = Submachine;
                using next_state_type = Submachine;
                using transition_event = Event;

                static process_result execute(
                    StateMachine& sm,
                    [[maybe_unused]] int& state_id,
                    Event const& event)
                {
                    BOOST_ASSERT(state_id == StateMachine::template get_state_id<Submachine>());
                    process_result result =
                        sm.template get_state<Submachine>().process_event_internal(event);
                    return result;
                }
            };

            template <typename Submachine>
            using initial_map_item = mp11::mp_list<
                Submachine,
                mp11::mp_list<forward_transition<Submachine>>>;

            // Class used to execute a chain of transitions for a given event and state.
            // Handles transition conflicts.
            template <typename State, typename Transitions>
            struct transition_chain
            {
                using current_state_type = State;
                using transition_event = Event;

                static process_result execute(StateMachine& sm, int& state_id, Event const& evt)
                {
                    process_result result = process_result::HANDLED_FALSE;
                    mp_for_each_until<Transitions>(
                        [&result, &sm, &state_id, &evt](auto transition)
                        {
                            using Transition = decltype(transition);
                            result |= Transition::execute(sm, state_id, evt);
                            if (result & handled_or_deferred)
                            {
                                // If a guard rejected previously, ensure this bit is not present.
                                result &= handled_or_deferred;
                                return true;
                            }
                            return false;
                        }
                    );
                    return result;
                }
            };

            // Merge each list of transitions into a chain if needed.
            template <typename State, typename Transitions>
            struct merge_transitions_impl;
            template <typename State, typename Transition>
            struct merge_transitions_impl<State, mp11::mp_list<Transition>>
            {
                using type = Transition;
            };
            template <typename State, typename... Transitions>
            struct merge_transitions_impl<State, mp11::mp_list<Transitions...>>
            {
                using type = transition_chain<State, mp11::mp_list<Transitions...>>;
            };
            template <typename StateAndTransitions>
            using merge_transitions = typename merge_transitions_impl<
                mp11::mp_first<StateAndTransitions>,
                mp11::mp_second<StateAndTransitions>>::type;

            // Convert the merged transitions into
            // initializer cell constants for the dispatch table.
            template <typename Transition,
                      bool IsKleeneEvent = is_kleene_event<
                          typename Transition::transition_event>::value>
            struct get_init_cell_constant_impl;
            template <typename Transition>
            struct get_init_cell_constant_impl<Transition, /*IsKleeneEvent=*/false>
            {
                using type = init_cell_constant<
                    // Offset into the entries array
                    StateMachine::template get_state_id<typename Transition::current_state_type>(),
                    // Address of the execute function
                    &Transition::execute>;
            };
            template <typename Transition>
            struct get_init_cell_constant_impl<Transition, /*IsKleeneEvent=*/true>
            {
                using type = init_cell_constant<
                    // Offset into the entries array
                    StateMachine::template get_state_id<typename Transition::current_state_type>(),
                    // Address of the execute function
                    &convert_event_and_execute<Transition>>;
            };
            template <typename Transition>
            using get_init_cell_constant = typename get_init_cell_constant_impl<Transition>::type;

            cell_t m_cells[max_state]{};
        };

        // "Dispatch" for a specific event (sm-internal).
        // A SM's internal transition table gets transformed into one
        // transition (chain), it can be executed immediately.
        class internal_dispatch_impl
        {
          private:
            // Class used to execute a chain of sm-internal transitions for a given event.
            // Handles transition conflicts.
            template <typename Transitions>
            struct internal_transition_chain
            {
                using transition_event = Event;

                static process_result execute(StateMachine& sm, Event const& evt)
                {
                    process_result result = process_result::HANDLED_FALSE;
                    mp_for_each_until<Transitions>(
                        [&result, &sm, &evt](auto transition)
                        {
                            using Transition = decltype(transition);
                            result |= Transition::execute(sm, evt);
                            if (result & handled_or_deferred)
                            {
                                // If a guard rejected previously,
                                // ensure this bit is not present.
                                result &= handled_or_deferred;
                                return true;
                            }
                            return false;
                        }
                    );
                    return result;
                }
            };

            // Helpers for row processing
            template <typename Transitions>
            struct transition_impl
            {
                using type = internal_transition_chain<Transitions>;
            };
            template <typename Transition>
            struct transition_impl<mp11::mp_list<Transition>>
            {
                using type = Transition;
            };
            template <typename Transition>
            using transition_event_predicate = transition_event_predicate<Transition>;
            using filtered_transitions =
                mp11::mp_copy_if<internal_transition_table<StateMachine>,
                                 transition_event_predicate>;

          public:
            using transition = typename transition_impl<filtered_transitions>::type;
        };
    };
};

} // detail
}}} // boost::msm::backmp11


#endif // BOOST_MSM_BACKMP11_DETAIL_FAVOR_RUNTIME_SPEED_H
