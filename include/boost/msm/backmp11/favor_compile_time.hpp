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

#ifndef BOOST_MSM_BACKMP11_FAVOR_COMPILE_TIME_H
#define BOOST_MSM_BACKMP11_FAVOR_COMPILE_TIME_H

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/msm/front/completion_event.hpp>
#include <boost/msm/backmp11/detail/metafunctions.hpp>
#include <boost/msm/backmp11/detail/dispatch_table.hpp>
#include <boost/msm/backmp11/detail/transition_table.hpp>
#include <boost/msm/backmp11/event_traits.hpp>

// Macro to generate the dispatch_table for a specific state machine type.
#define BOOST_MSM_BACKMP11_GENERATE_STATE_MACHINE(SM_TYPE)                     \
    namespace boost::msm::backmp11::detail                                     \
    {                                                                          \
    template <>                                                                \
    const typename compile_policy_impl<                                        \
        favor_compile_time>::template dispatch_table<SM_TYPE, any_event>&      \
    compile_policy_impl<favor_compile_time>::dispatch_table<                   \
        SM_TYPE, any_event>::instance()                                        \
    {                                                                          \
        static dispatch_table table;                                           \
        return table;                                                          \
    }                                                                          \
    } // boost::msm::backmp11::detail

namespace boost { namespace msm { namespace backmp11
{

struct favor_compile_time
{
    // TODO fix adapter and remove this.
    using compile_policy = int;
};

namespace detail
{

template <typename Policy>
struct compile_policy_impl;
template <>
struct compile_policy_impl<favor_compile_time>
{
// GCC cannot recognize the parameter pack in the array's initializer.
#if !defined(__GNUC__) || defined(__clang__)
    // Convert a list with integral constants of the same value_type to a value array
    // (non-constexpr version required due to typeid).
    template <typename List>
    struct value_array;
    template <typename... Ts>
    struct value_array<mp11::mp_list<Ts...>>
    {
        using value_type =
            typename mp11::mp_front<mp11::mp_list<Ts...>>::value_type;
        const value_type value[sizeof...(Ts)]{Ts{}.value...};
    };
#endif

    template <typename Event>
    static any_event normalize_event(const Event& event)
    {
        return any_event{event};
    }

    constexpr static const any_event& normalize_event(const any_event& event)
    {
        return event;
    }

    static bool is_completion_event(const any_event& event)
    {
        return (event.type() == typeid(front::none));
    }

    template<typename Statemachine>
    static bool is_end_interrupt_event(Statemachine& sm, const any_event& event)
    {
        static end_interrupt_event_helper helper{sm};
        return helper.is_end_interrupt_event(event);
    }

    // Dispatch table for event deferral checks.
    // Converts any_events and calls 'is_event_deferred' on states.
    template <typename StateMachine>
    class is_event_deferred_dispatch_table
    {
      public:
        template <typename State>
        static bool dispatch(const StateMachine& sm, const State& state,
                             const any_event& event)
        {
            static const is_event_deferred_dispatch_table table{state};
            auto it = table.m_cells.find(event.type());
            if (it != table.m_cells.end())
            {
                using real_cell =
                    bool (*)(const StateMachine&, const State&, const any_event&);
                auto cell = reinterpret_cast<real_cell>(it->second);
                return (*cell)(sm, state, event);
            }
            return false;
        }

      private:
        template <typename State>
        is_event_deferred_dispatch_table(const State&)
        {
            using deferred_events = to_mp_list_t<typename State::deferred_events>;
            using deferred_event_identities =
                mp11::mp_transform<mp11::mp_identity, deferred_events>;
            mp11::mp_for_each<deferred_event_identities>(
                [this](auto event_identity)
                {
                    using Event = typename decltype(event_identity)::type;
                    m_cells[to_type_index<Event>()] =
                        reinterpret_cast<generic_cell>(&convert_and_execute<State, Event>);
                });
        }

        template<typename State, typename Event>
        static bool convert_and_execute(const StateMachine& sm, const State& state,
                                        const any_event& event)
        {
            return state.is_event_deferred(*any_cast<Event>(&event), sm.get_fsm_argument());
        }

        std::unordered_map<std::type_index, generic_cell> m_cells;
    };

    template <typename StateMachine>
    struct is_event_deferred_visitor
    {
        template <typename State>
        using predicate = has_deferred_events<State>;

        is_event_deferred_visitor(const StateMachine& sm, const any_event& event)
            : sm(sm), event(event)
        {
        }

        template <typename State>
        void operator()(const State& state)
        {
            using table = is_event_deferred_dispatch_table<StateMachine>;
            result |= table::dispatch(sm, state, event);
        }

        const StateMachine& sm;
        const any_event& event;
        bool result{false};
    };

    template <typename StateMachine>
    static bool is_event_deferred(const StateMachine& sm, const any_event& event)
    {
        if constexpr (
            !mp11::mp_empty<typename StateMachine::deferring_states>::value)
        {
            using visitor_t = is_event_deferred_visitor<StateMachine>;
            visitor_t visitor{sm, event};
            sm.template visit_if<visit_mode::active_non_recursive,
                                 visitor_t::template predicate>(visitor);
            return visitor.result;
        }
        else
        {
            return false;
        }
    }

    template <typename StateMachine>
    static void defer_event(StateMachine& sm, any_event const& event,
                            bool next_rtc_seq)
    {
        sm.do_defer_event(event, next_rtc_seq);
    }

    // Convert an event to a type index.
    template<class Event>
    static std::type_index to_type_index()
    {
        return std::type_index{typeid(Event)};
    }

    // Helper class to manage end interrupt events.
    class end_interrupt_event_helper
    {
      public:
        template<class StateMachine>
        end_interrupt_event_helper(const StateMachine& sm)
        {
            using event_set = generate_event_set<
                typename StateMachine::front_end_t::transition_table>;
            mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, event_set>>(
                [this, &sm](auto event_identity)
                {
                    using Event = typename decltype(event_identity)::type;
                    using Flag = EndInterruptFlag<Event>;
                    m_is_flag_active_functions[to_type_index<Event>()] =
                        [&sm](){return sm.template is_flag_active<Flag>();};
                });
        }

        bool is_end_interrupt_event(const any_event& event) const
        {
            auto it = m_is_flag_active_functions.find(event.type());
            if (it != m_is_flag_active_functions.end())
            {
                return (it->second)();
            }
            return false;
        }

      private:
        using map = std::unordered_map<std::type_index, std::function<bool()>>;
        map m_is_flag_active_functions;
    };

    // Class used to build a chain of transitions for a given event and state.
    // Allows transition conflicts.
    class transition_chain
    {
      public:
        template<typename StateMachine>
        process_result execute(StateMachine& sm, int& state_id, any_event const& event) const
        {
            using cell_t = process_result (*)(StateMachine&, int&, any_event const&);
            process_result result = process_result::HANDLED_FALSE;
            for (const generic_cell cell : m_transition_cells)
            {
                result |= reinterpret_cast<cell_t>(cell)(sm, state_id, event);
                if (result & handled_true_or_deferred)
                {
                    // If a guard rejected previously, ensure this bit is not present.
                    return result & handled_true_or_deferred;
                }
            }
            // At this point result can be HANDLED_FALSE or HANDLED_GUARD_REJECT.
            return result;
        }

        template <typename StateMachine>
        void add_transition_cell(process_result (*cell)(StateMachine&, int&, any_event const&))
        {
            m_transition_cells.emplace_back(
                reinterpret_cast<generic_cell>(cell));
        }

      private:
        std::vector<generic_cell> m_transition_cells;
    };

    // Class used to build a chain of sm-internal transitions for a given event and state.
    // Allows transition conflicts.
    class internal_transition_chain
    {
      public:
        template<typename StateMachine>
        process_result execute(StateMachine& sm, any_event const& event) const
        {
            using cell_t = process_result (*)(StateMachine&, any_event const&);
            process_result result = process_result::HANDLED_FALSE;
            for (const generic_cell cell : m_transition_cells)
            {
                result |= reinterpret_cast<cell_t>(cell)(sm, event);
                if (result & handled_true_or_deferred)
                {
                    // If a guard rejected previously, ensure this bit is not present.
                    return result & handled_true_or_deferred;
                }
            }
            // At this point result can be HANDLED_FALSE or HANDLED_GUARD_REJECT.
            return result;
        }

        template <typename StateMachine>
        void add_transition_cell(process_result (*cell)(StateMachine&, any_event const&))
        {
            m_transition_cells.emplace_back(
                reinterpret_cast<generic_cell>(cell));
        }

      private:
        std::vector<generic_cell> m_transition_cells;
    };

    // Generates a singleton runtime lookup table that maps current state
    // to a function that makes the SM take its transition on the given
    // Event type.
    template<class StateMachine, typename Event>
    class dispatch_table;
    template<class StateMachine>
    class dispatch_table<StateMachine, any_event>
    {
      public:
        // Dispatch an event.
        static process_result dispatch(StateMachine& sm, int& state_id, const any_event& event)
        {
            const dispatch_table& self = instance();
            return self.m_state_dispatch_tables[state_id].dispatch(sm, state_id, event);
        }

        // Dispatch an event to the SM's internal table.
        static process_result internal_dispatch(StateMachine& sm, const any_event& event)
        {
            if constexpr (has_internal_transitions::value)
            {
                const dispatch_table& self = instance();
                return self.m_internal_dispatch_table.dispatch(sm, event);
            }
            return process_result::HANDLED_FALSE;
        }

      private:
        using state_set = typename StateMachine::internal::state_set;
        using submachines = mp11::mp_copy_if<state_set, has_state_machine_tag>;
        
        // Value used to initialize a cell of the dispatch table.
        using cell_t = process_result (*)(StateMachine&, int&, any_event const&);
        struct init_cell_value
        {
            std::type_index event_type_index;
            size_t state_id;
            cell_t cell;
        };
        template<typename Event, size_t index, cell_t cell>
        struct init_cell_constant
        {
            using value_type = init_cell_value;
            const value_type value = {typeid(Event), index, cell};
        };

        template<typename Event, typename Transition>
        static process_result convert_event_and_execute(
            StateMachine& sm, int& state_id, const any_event& event)
        {
            return Transition::execute(sm, state_id, *any_cast<Event>(&event));
        }

        template <typename Transition>
        struct get_init_cell_constant_impl
        {
            using type = init_cell_constant<
                typename Transition::transition_event,
                StateMachine::template get_state_id<typename Transition::current_state_type>(),
                convert_event_and_execute<typename Transition::transition_event, Transition>
                >;
        };
        template<typename Transition>
        using get_init_cell_constant = typename get_init_cell_constant_impl<Transition>::type;

        using has_internal_transitions =
            mp11::mp_not<mp11::mp_empty<internal_transition_table<StateMachine>>>;
        using has_transitions =
            mp11::mp_not<mp11::mp_empty<transition_table<StateMachine>>>;

        // Dispatch table for one state.
        class state_dispatch_table
        {
          public:
            // Initialize the call to the composite state's process_event function.
            template<typename CompositeState>
            void init_composite_state()
            {
                m_call_process_event = &call_process_event<CompositeState>;
            }

            // Add a cell to the dispatch table.
            void add_transition_cell(const init_cell_value& value)
            {
                transition_chain& chain = m_transition_chains[value.event_type_index];
                chain.add_transition_cell(value.cell);
            }

            // Dispatch an event.
            process_result dispatch(StateMachine& sm, int& state_id, const any_event& event) const
            {
                process_result result = process_result::HANDLED_FALSE;
                if (m_call_process_event)
                {
                    result = m_call_process_event(sm, event);
                    if (result)
                    {
                        return result;
                    }
                }
                auto it = m_transition_chains.find(event.type());
                if (it != m_transition_chains.end())
                {
                    result = (it->second.execute)(sm, state_id, event);
                }
                return result;
            }

          private:
            template <typename Submachine>
            static process_result call_process_event(StateMachine& sm, const any_event& event)
            {
                return sm.template get_state<Submachine&>()
                    .process_event_internal(event, process_info::submachine_call);
            }

            std::unordered_map<std::type_index, transition_chain> m_transition_chains;
            // Optional method in case the state is a composite.
            process_result (*m_call_process_event)(StateMachine&, const any_event&){nullptr};
        };

        // Value used to initialize a cell of the sm-internal dispatch table.
        using internal_cell_t = process_result (*)(StateMachine&, any_event const&);
        struct init_internal_cell_value
        {
            std::type_index event_type_index;
            internal_cell_t cell;
        };
        template<typename Event, internal_cell_t cell>
        struct init_internal_cell_constant
        {
            using value_type = init_internal_cell_value;
            const value_type value = {typeid(Event), cell};
        };

        template<typename Event, typename Transition>
        static process_result convert_event_and_execute_internal(
            StateMachine& sm, const any_event& event)
        {
            return Transition::execute(sm, *any_cast<Event>(&event));
        }

        template <typename Transition>
        struct get_internal_init_cell_constant_impl
        {
            using type = init_internal_cell_constant<
                typename Transition::transition_event,
                convert_event_and_execute_internal<typename Transition::transition_event, Transition>
                >;
        };
        template <typename Transition>
        using get_internal_init_cell_constant =
            typename get_internal_init_cell_constant_impl<Transition>::type;

        // Dispatch table for sm-internal transitions.
        class internal_dispatch_table
        {
          public:
            // Add a cell to the dispatch table.
            void add_transition_cell(const init_internal_cell_value& value)
            {
                internal_transition_chain& chain =
                    m_transition_chains[value.event_type_index];
                chain.add_transition_cell(value.cell);
            }

            // Dispatch an event.
            process_result dispatch(StateMachine& sm, const any_event& event) const
            {
                process_result result = process_result::HANDLED_FALSE;
                auto it = m_transition_chains.find(event.type());
                if (it != m_transition_chains.end())
                {
                    result = (it->second.execute)(sm, event);
                }
                return result;
            }

          private:
            std::unordered_map<std::type_index, internal_transition_chain>
                m_transition_chains;
        };

        dispatch_table()
        {
            // Initialize the state dispatch tables.
            using submachines = mp11::mp_copy_if<state_set, has_state_machine_tag>;
            mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, submachines>>(
                [this](auto state_identity)
                {
                    using Submachine = typename decltype(state_identity)::type;
                    static constexpr int state_id =
                        StateMachine::template get_state_id<Submachine>();
                    m_state_dispatch_tables[state_id].template init_composite_state<Submachine>();
                });
            if constexpr (has_transitions::value)
            {
                using init_cell_constants = mp11::mp_transform<
                    get_init_cell_constant,
                    transition_table<StateMachine>>;
#if defined(__GNUC__) && !defined(__clang__)
                mp11::mp_for_each<init_cell_constants>(
                    [this](auto constant)
                    {
                        m_state_dispatch_tables[constant.value.state_id].add_transition_cell(constant.value);
                    });
#else
                value_array<init_cell_constants> value_array;
                for (const init_cell_value& value: value_array.value)
                {
                    m_state_dispatch_tables[value.state_id].add_transition_cell(value);
                }
#endif
            }

            // Initialize the sm-internal dispatch table.
            if constexpr (has_internal_transitions::value)
            {
                using init_internal_cell_constants = mp11::mp_transform<
                    get_internal_init_cell_constant,
                    internal_transition_table<StateMachine>>;
#if defined(__GNUC__) && !defined(__clang__)
                mp11::mp_for_each<init_internal_cell_constants>(
                    [this](auto constant)
                    {
                        m_internal_dispatch_table.add_transition_cell(constant.value);
                    });
#else
                value_array<init_internal_cell_constants> value_array;
                for (const init_internal_cell_value& value: value_array.value)
                {
                    m_internal_dispatch_table.add_transition_cell(value);
                }
#endif
            }
        }

        // The singleton instance.
        static const dispatch_table& instance();

        // We need one dispatch table per state.
        state_dispatch_table m_state_dispatch_tables[mp11::mp_size<state_set>::value];
        internal_dispatch_table m_internal_dispatch_table;
    };
};

#ifndef BOOST_MSM_BACKMP11_MANUAL_GENERATION

template<class StateMachine>
const typename compile_policy_impl<favor_compile_time>::template
    dispatch_table<StateMachine, any_event>& 
compile_policy_impl<favor_compile_time>::dispatch_table<StateMachine, any_event>::instance()
{
    static dispatch_table table;
    return table;
}

#endif

} // detail
}}} // boost::msm::backmp11

#endif //BOOST_MSM_BACKMP11_FAVOR_COMPILE_TIME_H
