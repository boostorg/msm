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
    using add_forwarding_rows = mp11::mp_true;

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
            typedef typename generate_event_set<typename StateMachine::internal::stt>::event_set_mp11 event_list;
            bool found =
                mp_for_each_until<mp11::mp_transform<mp11::mp_identity, event_list>>(
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

    struct cell_initializer
    {
        static void init(generic_cell* entries, const generic_init_cell_value* array, size_t size)
        {
            for (size_t i=0; i<size; i++)
            {
                const auto& item = array[i];
                entries[item.index] = item.address;
            }
        }
    };

    template<typename StateMachine, typename State, bool StateIsStateMachine = std::is_same_v<StateMachine, State>>
    struct get_table_index_impl;
    template<typename StateMachine, typename State>
    struct get_table_index_impl<StateMachine, State, false>
    {
        using type = mp11::mp_size_t<StateMachine::template get_state_id<State>() + 1>;
    };
    template<typename StateMachine, typename State>
    struct get_table_index_impl<StateMachine, State, true>
    {
        using type = mp11::mp_size_t<0>;
    };
    template<typename StateMachine, typename State>
    using get_table_index = typename get_table_index_impl<StateMachine, State>::type;

    // Generates a singleton runtime lookup table that maps current state
    // to a function that makes the SM take its transition on the given
    // Event type.
    template<class StateMachine>
    class dispatch_table
    {
        using Stt = typename StateMachine::complete_table;
    public:
        // Dispatch function for a specific event.
        template<class Event>
        using cell = process_result (*)(StateMachine&, int&, Event const&);

        // Dispatch an event.
        template<class Event>
        static process_result dispatch(StateMachine& sm, int& state_id, const Event& event)
        {
            return event_dispatch_table<Event>::instance().entries[state_id+1](sm, state_id, event);
        }

        // Dispatch an event to the FSM's internal table.
        template<class Event>
        static process_result dispatch_internal(StateMachine& sm, const Event& event)
        {
            int no_state_id;
            return event_dispatch_table<Event>::instance().entries[0](sm, no_state_id, event);
        }

    private:
        // Compute the maximum state value in the sm so we know how big
        // to make the tables.
        using state_set = typename StateMachine::internal::state_set;
        static constexpr int max_state = mp11::mp_size<state_set>::value;

        // Dispatch table for a specific event.
        template<class Event>
        class event_dispatch_table
        {
        public:
            using event_cell = cell<Event>;

            // The singleton instance.
            static const event_dispatch_table& instance() {
                static event_dispatch_table table;
                return table;
            }

        private:
            static process_result execute_no_transition(StateMachine&, int&, const Event&)
            {
                return process_result::HANDLED_FALSE;
            }

            // Initialize the dispatch table for the event
            event_dispatch_table()
            {
                // Initialize cells for no transition
                for (size_t i=0;i<max_state+1; i++)
                {
                    entries[i] = &execute_no_transition;
                }

                // build chaining rows for rows coming from the same state and the current event
                // first we build a map of sequence for every source
                // in reverse order so that the frow's are handled first (UML priority)
                typedef mp11::mp_fold<
                    mp11::mp_copy_if<
                        to_mp_list_t<Stt>,
                        event_filter_predicate
                        >,
                    mp11::mp_list<>,
                    map_updater
                    > map_of_row_seq;
                // and then build chaining rows for all source states having more than 1 row
                typedef mp11::mp_transform<
                    transition_chainer,
                    map_of_row_seq
                    > chained_rows;

                // Go back and fill in cells for matching transitions.
                typedef mp11::mp_transform<
                    preprocess_transition,
                    chained_rows
                    > chained_and_preprocessed_rows;
                event_cell_initializer::init(
                    reinterpret_cast<generic_cell*>(entries),
                    get_init_cells<event_cell, chained_and_preprocessed_rows>(),
                    mp11::mp_size<chained_and_preprocessed_rows>::value
                    );
            }
            
            // Class used to build a chain of transitions for a given event and state.
            // Allows transition conflicts.
            template<typename State, typename Transitions>
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

            template <
                typename State,
                typename FilteredTransitionTable,
                bool MoreThanOneFrow = (mp11::mp_count_if<FilteredTransitionTable, has_is_frow>::value > 1)>
            struct make_transition_chain_impl;
            template <typename State, typename FilteredTransitionTable>
            struct make_transition_chain_impl<State, FilteredTransitionTable, false>
            {
                using type = transition_chain<State, FilteredTransitionTable>;
            };
            template <typename State, typename FilteredTransitionTable>
            struct make_transition_chain_impl<State, FilteredTransitionTable, true>
            {
                // if we have more than one frow with the same state as source, remove the ones extra
                // note: we know the frows are located at the beginning so we remove at the beginning
                // (number of frows - 1) elements
                static constexpr size_t number_frows =
                    boost::mp11::mp_count_if<FilteredTransitionTable, has_is_frow>::value;
                using type =
                    transition_chain<State, mp11::mp_drop_c<FilteredTransitionTable, number_frows - 1>>;
            };
            template <typename State, typename FilteredTransitionTable>
            using make_transition_chain = typename make_transition_chain_impl<State, FilteredTransitionTable>::type;

            template <typename Transition>
            static process_result convert_event_and_execute(StateMachine& sm, int& state_id, Event const& evt)
            {
                typename Transition::transition_event kleene_event{evt};
                return Transition::execute(sm, state_id, kleene_event);
            }

            using event_init_cell_value = init_cell_value<event_cell>;

            template<size_t v1, event_cell v2>
            using init_cell_constant = init_cell_constant<v1, event_cell, v2>;

            template<event_cell v>
            using cell_constant = std::integral_constant<event_cell, v>;

            using event_cell_initializer = cell_initializer;

            // Helpers for row processing
            // First operation (fold)
            template <typename T, bool IsKleeneEvent = is_kleene_event<typename T::transition_event>::value>
            struct event_filter_predicate_impl
            {
                using type = std::is_base_of<typename T::transition_event, Event>;
            };
            template <typename T>
            struct event_filter_predicate_impl<T, true>
            {
                using type = mp11::mp_true;
            };
            template <typename T>
            using event_filter_predicate = 
                typename event_filter_predicate_impl<T>::type;

            // Changes the event type for a frow to the event we are dispatching.
            // This helps ensure that an event does not get processed more than once
            // because of frows and base events.
            template <typename Transition, bool IsFrow = has_is_frow<Transition>::value>
            struct normalize_transition_impl;
            template <typename Transition>
            struct normalize_transition_impl<Transition, false>
            {
                using type = Transition;
            };
            template <typename Transition>
            struct normalize_transition_impl<Transition, true>
            {
                using type = typename Transition::template replace_event<Event>;
            };
            template <typename Transition>
            using normalize_transition = typename normalize_transition_impl<Transition>::type;

            template <typename M, typename Key, typename Value>
            using push_map_value = mp11::mp_push_front<
                mp11::mp_second<mp11::mp_map_find<M, Key>>,
                Value>;
            template<typename Map, typename Transition, bool FirstEntry = !mp11::mp_map_contains<Map, typename Transition::current_state_type>::value>
            struct map_updater_impl;
            template<typename Map, typename Transition>
            struct map_updater_impl<Map, Transition, false>
            {
                using type = mp11::mp_map_replace<
                    Map,
                    // list already exists, add the row
                    mp11::mp_list<
                        typename Transition::current_state_type,
                        push_map_value<
                            Map,
                            typename Transition::current_state_type,
                            normalize_transition<Transition>
                        >
                    >
                >;
            };
            template<typename Map, typename Transition>
            struct map_updater_impl<Map, Transition, true>
            {
                using type = mp11::mp_map_replace<
                    Map,
                    mp11::mp_list<
                        typename Transition::current_state_type,
                        // first row on this source state, make a list with 1 element
                        mp11::mp_list<normalize_transition<Transition>>
                    >
                >;
            };
            template<typename Map, typename Transition>
            using map_updater = typename map_updater_impl<Map, Transition>::type;

            // Second operation (transform)
            template<
                typename StateAndFilteredTransitionTable,
                bool MultipleTransitions = (mp11::mp_size<mp11::mp_second<StateAndFilteredTransitionTable>>::value > 1)>
            struct transition_chainer_impl;
            template<typename StateAndFilteredTransitionTable>
            struct transition_chainer_impl<StateAndFilteredTransitionTable, false>
            {
                // just one row, no chaining, we rebuild the row like it was before
                using type = mp11::mp_front<mp11::mp_second<StateAndFilteredTransitionTable>>;
            };
            template<typename StateAndFilteredTransitionTable>
            struct transition_chainer_impl<StateAndFilteredTransitionTable, true>
            {
                // we need row chaining
                using type = make_transition_chain<
                    mp11::mp_first<StateAndFilteredTransitionTable>,
                    mp11::mp_second<StateAndFilteredTransitionTable>>;
            };
            template<typename StateAndFilteredTransitionTable>
            using transition_chainer = typename transition_chainer_impl<StateAndFilteredTransitionTable>::type;
            template<typename Transition, bool IsKleeneEvent = is_kleene_event<typename Transition::transition_event>::value>
            struct preprocess_transition_impl;
            template<typename Transition>
            struct preprocess_transition_impl<Transition, false>
            {
                using type = init_cell_constant<
                    // Offset into the entries array
                    get_table_index<StateMachine, typename Transition::current_state_type>::value,
                    // Address of the execute function
                    cell_constant<&Transition::execute>::value
                    >;
            };
            template<typename Transition>
            struct preprocess_transition_impl<Transition, true>
            {
                using type = init_cell_constant<
                    // Offset into the entries array
                    get_table_index<StateMachine, typename Transition::current_state_type>::value,
                    // Address of the execute function
                    cell_constant<&convert_event_and_execute<Transition>>::value
                    >;
            };
            template<typename Transition>
            using preprocess_transition = typename preprocess_transition_impl<Transition>::type;

        // data members
        public:
            // max_state+1, because 0 is reserved for this sm (internal transitions)
            event_cell entries[max_state+1];
        };
    };
};

} // detail

}}} // boost::msm::backmp11


#endif // BOOST_MSM_BACKMP11_DETAIL_FAVOR_RUNTIME_SPEED_H
