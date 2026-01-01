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

#include <deque>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

#include <boost/msm/front/completion_event.hpp>
#include <boost/msm/backmp11/detail/metafunctions.hpp>
#include <boost/msm/backmp11/detail/dispatch_table.hpp>
#include <boost/msm/backmp11/event_traits.hpp>

#define BOOST_MSM_BACKMP11_GENERATE_STATE_MACHINE(smname)                      \
    template<>                                                                  \
    const smname::sm_dispatch_table& smname::sm_dispatch_table::instance()      \
    {                                                                           \
        static dispatch_table table;                                            \
        return table;                                                           \
    }

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
    using add_forwarding_rows = mp11::mp_false;

    // Bitmask for process result checks.
    static constexpr process_result handled_or_deferred =
        process_result::HANDLED_TRUE | process_result::HANDLED_DEFERRED;

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

    template <typename Event>
    static any_event normalize_event(const Event& event)
    {
        return any_event{event};
    }

    constexpr static const any_event& normalize_event(const any_event& event)
    {
        return event;
    }

    template <typename StateMachine>
    class deferred_event_impl : public deferred_event
    {
      public:
        deferred_event_impl(StateMachine& sm, const any_event& event, size_t seq_cnt)
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
        any_event m_event;
    };

    // For this policy we cannot determine whether a specific event
    // is deferred at compile-time, as all events are any_events.
    template <typename StateOrStates, typename Event>
    using has_deferred_event = has_deferred_events<StateOrStates>;

    template <typename State>
    static const std::unordered_set<std::type_index>& get_deferred_event_type_indices()
    {
        static std::unordered_set<std::type_index> type_indices = []()
        {
            std::unordered_set<std::type_index> indices;
            using deferred_events = to_mp_list_t<typename State::deferred_events>;
            using deferred_event_identities = mp11::mp_transform<mp11::mp_identity, deferred_events>;
            mp11::mp_for_each<deferred_event_identities>(
                [&indices](auto event_identity)
                {
                    using Event = typename decltype(event_identity)::type;
                    indices.emplace(to_type_index<Event>());
                }
            );
            return indices;
        }();
        return type_indices;
    }

    template <typename StateMachine>
    static bool is_event_deferred(const StateMachine& sm, const any_event& event)
    {
        bool result = false;
        const std::type_index type_index = event.type();
        auto visitor = [&result, type_index](const auto& state)
        {
            using State = std::decay_t<decltype(state)>;
            const auto& set = get_deferred_event_type_indices<State>();
            result |= (set.find(type_index) != set.end());
        };
        sm.template visit_if<visit_mode::active_non_recursive,
                             has_deferred_events>(visitor);
        return result;
    }

    template <typename StateMachine>
    static void defer_event(StateMachine& sm, any_event const& event)
    {
        auto& deferred_events = sm.get_deferred_events();
        deferred_events.queue.push_back(basic_unique_ptr<deferred_event>{
            new deferred_event_impl(sm, event, deferred_events.cur_seq_cnt)});
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
            typedef typename generate_event_set<typename StateMachine::internal::stt>::event_set_mp11 event_set_mp11;
            mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, event_set_mp11>>(
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
            using real_cell = process_result (*)(StateMachine&, int&, any_event const&);
            process_result result = process_result::HANDLED_FALSE;
            for (const generic_cell function : m_transition_functions)
            {
                result |= reinterpret_cast<real_cell>(function)(sm, state_id, event);
                if (result & handled_or_deferred)
                {
                    // If a guard rejected previously, ensure this bit is not present.
                    return result & handled_or_deferred;
                }
            }
            // At this point result can be HANDLED_FALSE or HANDLED_GUARD_REJECT.
            return result;
        }

        template <typename StateMachine, typename Event, typename Transition>
        void add_transition()
        {
            m_transition_functions.emplace_front(reinterpret_cast<generic_cell>(
                &convert_event_and_execute<StateMachine, Event, Transition>));
        }

      private:
        // Adapter for calling a transition's execute function.
        template<typename StateMachine, typename Event, typename Transition>
        static process_result convert_event_and_execute(
            StateMachine& sm, int& state_id, const any_event& event)
        {
            return Transition::execute(sm, state_id, *any_cast<Event>(&event));
        }

        std::deque<generic_cell> m_transition_functions;
    };

    // Generates a singleton runtime lookup table that maps current state
    // to a function that makes the SM take its transition on the given
    // Event type.
    template<class StateMachine>
    class dispatch_table
    {
    public:
        // Dispatch an event.
        static process_result dispatch(StateMachine& sm, int& state_id, const any_event& event)
        {
            return instance().m_state_dispatch_tables[state_id+1].dispatch(sm, state_id, event);
        }

        // Dispatch an event to the SM's internal table.
        static process_result dispatch_internal(StateMachine& sm, const any_event& event)
        {
            int no_state_id;
            return instance().m_state_dispatch_tables[0].dispatch(sm, no_state_id, event);
        }

    private:
        using state_set = typename StateMachine::internal::state_set;
        using Stt = typename StateMachine::complete_table;

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

            // Add a transition to the dispatch table.
            template<typename Event, typename Transition>
            void add_transition()
            {
                transition_chain& chain = m_transition_chains[to_type_index<Event>()];
                chain.add_transition<StateMachine, Event, Transition>();
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
            template <typename CompositeState>
            static process_result call_process_event(StateMachine& sm, const any_event& event)
            {
                return sm.template get_state<CompositeState&>().process_event_internal(event);
            }

            std::unordered_map<std::type_index, transition_chain> m_transition_chains;
            // Optional method in case the state is a composite.
            process_result (*m_call_process_event)(StateMachine&, const any_event&){nullptr};
        };

        dispatch_table()
        {
            // Execute row-specific initializations.
            mp11::mp_for_each<Stt>(
                [this](auto transition)
                {
                    using Transition = decltype(transition);
                    using Event = typename Transition::transition_event;
                    using State = typename Transition::current_state_type;
                    static constexpr int state_id = StateMachine::template get_state_id<State>();
                    m_state_dispatch_tables[state_id + 1].template add_transition<Event, Transition>();
                });

            // Execute state-specific initializations.
            using submachine_states = mp11::mp_copy_if<state_set, has_state_machine_tag>;
            mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, submachine_states>>(
                [this](auto state_identity)
                {
                    using CompositeState = typename decltype(state_identity)::type;
                    static constexpr int state_id = StateMachine::template get_state_id<CompositeState>();
                    m_state_dispatch_tables[state_id + 1].template init_composite_state<CompositeState>();
                });
        }

        // The singleton instance.
        static const dispatch_table& instance();

        // We need one dispatch table per state, plus one for internal transitions of the SM.
        BOOST_STATIC_CONSTANT(int, max_state = (mp11::mp_size<state_set>::value));
        state_dispatch_table m_state_dispatch_tables[max_state+1];
    };
};

#ifndef BOOST_MSM_BACKMP11_MANUAL_GENERATION

template<class StateMachine>
const typename compile_policy_impl<favor_compile_time>::template dispatch_table<StateMachine>& 
compile_policy_impl<favor_compile_time>::dispatch_table<StateMachine>::instance()
{
    static dispatch_table table;
    return table;
}

#endif


} // detail

}}} // boost::msm::backmp11

#endif //BOOST_MSM_BACKMP11_FAVOR_COMPILE_TIME_H
