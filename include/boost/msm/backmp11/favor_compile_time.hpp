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

#include <boost/msm/front/completion_event.hpp>
#include <boost/msm/backmp11/metafunctions.hpp>
#include <boost/msm/backmp11/dispatch_table.hpp>

namespace boost { namespace msm { namespace backmp11
{

#define BOOST_MSM_BACKMP11_GENERATE_DISPATCH_TABLE(fsmname)                     \
    template<>                                                                  \
    const fsmname::sm_dispatch_table& fsmname::sm_dispatch_table::instance()    \
    {                                                                           \
        static dispatch_table table;                                            \
        return table;                                                           \
    }

struct favor_compile_time
{
    typedef int compile_policy;
    typedef mp11::mp_false add_forwarding_rows;
};

template <class Event>
struct is_completion_event<Event, favor_compile_time>
{
    static bool value(const Event& event)
    {
        return (event.type() == typeid(front::none));
    }
};

template<typename Stt>
struct get_real_rows
{
    template<typename Transition>
    using is_real_row = mp11::mp_not<typename has_not_real_row_tag<Transition>::type>;
    typedef mp11::mp_copy_if<Stt, is_real_row> type;
};

// Convert an event to a type index.
template<class Event>
inline std::type_index to_type_index()
{
    return std::type_index{typeid(Event)};
}

// Helper class to manage end interrupt events.
class end_interrupt_event_helper
{
 public:
    template<class Fsm>
    end_interrupt_event_helper(const Fsm& fsm)
    {
        mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, typename Fsm::event_set_mp11>>(
            [this, &fsm](auto event_identity)
            {
                using Event = typename decltype(event_identity)::type;
                using Flag = EndInterruptFlag<Event>;
                m_is_flag_active_functions[to_type_index<Event>()] =
                    [&fsm](){return fsm.template is_flag_active<Flag>();};
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

struct chain_row
{
    template<typename Fsm>
    HandledEnum operator()(Fsm& fsm, int region, int state, any_event const& evt) const
    {
        typedef HandledEnum (*real_cell)(Fsm&, int, int, any_event const&);
        HandledEnum res = HANDLED_FALSE;
        typename std::deque<generic_cell>::const_iterator it = one_state.begin();
        while (it != one_state.end() && (res != HANDLED_TRUE && res != HANDLED_DEFERRED ))
        {
            auto fnc = reinterpret_cast<real_cell>(*it);
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
    // Use a deque with a generic type to avoid unnecessary template instantiations.
    std::deque<generic_cell> one_state;
};

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the SM take its transition on the given
// Event type.
template<class Fsm>
class dispatch_table<Fsm, favor_compile_time>
{
    using Stt = typename Fsm::complete_table;
public:
    // Dispatch an event.
    static HandledEnum dispatch(Fsm& fsm, int region_id, int state_id, const any_event& event)
    {
        return instance().m_state_dispatch_tables[state_id+1].dispatch(fsm, region_id, state_id, event);
    }

    // Dispatch an event to the FSM's internal table.
    static HandledEnum dispatch_internal(Fsm& fsm, int region_id, int state_id, const any_event& event)
    {
        return instance().m_state_dispatch_tables[0].dispatch(fsm, region_id, state_id, event);
    }

private:
    // Adapter for calling a row's execute function.
    template<typename Event, typename Row>
    static HandledEnum convert_and_execute(Fsm& fsm, int region_id, int state_id, const any_event& event)
    {
        return Row::execute(fsm, region_id, state_id, *any_cast<Event>(&event));
    }

    // Dispatch table for one state.
    class state_dispatch_table
    {
    public:
        // Initialize the table for the given state.
        template<typename State>
        void init()
        {
            // Fill in cells for deferred events.
            mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, to_mp_list_t<typename State::deferred_events>>>(
                [this](auto event_identity)
                {
                    using Event = typename decltype(event_identity)::type;
                    // using test = print_types<Event>;
                    auto& chain_row = get_chain_row<Event>();
                    chain_row.one_state.push_front(reinterpret_cast<generic_cell>(&Fsm::template defer_transition<any_event>));
                });

            if constexpr (has_back_end_tag<State>::value)
            {
                m_call_submachine = [](Fsm& fsm, const any_event& evt)
                {
                    return (fsm.template get_state<State&>()).process_event_internal(evt);
                };
            }
        }

        template<typename Event>
        chain_row& get_chain_row()
        {
            return m_entries[to_type_index<Event>()];
        }

        // Dispatch an event.
        HandledEnum dispatch(Fsm& fsm, int region_id, int state_id, const any_event& event) const
        {
            HandledEnum handled = HANDLED_FALSE;
            if (m_call_submachine)
            {
                handled = m_call_submachine(fsm, event);
                if (handled)
                {
                    return handled;
                }
            }
            auto it = m_entries.find(event.type());
            if (it != m_entries.end())
            {
                handled = (it->second)(fsm, region_id, state_id, event);
            }
            return handled;
        }

    private:
        std::unordered_map<std::type_index, chain_row> m_entries;
        // Special functor if the state is a composite
        std::function<HandledEnum(Fsm&, const any_event&)> m_call_submachine;
    };

    // Filter a state to check whether state-specific initialization
    // needs to be performed.
    template<typename State>
    using state_filter_predicate = mp11::mp_or<
        mp11::mp_not<mp11::mp_empty<to_mp_list_t<typename State::deferred_events>>>,
        has_back_end_tag<State>
        >;

    dispatch_table()
    {
        // Execute row-specific initializations.
        mp11::mp_for_each<typename get_real_rows<Stt>::type>(
            [this](auto row)
            {
                using Row = decltype(row);
                using Event = typename Row::transition_event;
                using State = typename Row::current_state_type;
                static constexpr int state_id = Fsm::template get_state_id<State>();
                auto& chain_row = m_state_dispatch_tables[state_id + 1].template get_chain_row<Event>();
                chain_row.one_state.push_front(reinterpret_cast<generic_cell>(&convert_and_execute<Event, Row>));
            });

        // Execute state-specific initializations.
        using filtered_states = mp11::mp_copy_if<state_set_mp11, state_filter_predicate>;
        mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, filtered_states>>(
            [this](auto state_identity)
            {
                using State = typename decltype(state_identity)::type;
                static constexpr int state_id = Fsm::template get_state_id<State>();
                m_state_dispatch_tables[state_id + 1].template init<State>();
            });
    }

    // The singleton instance.
    static const dispatch_table& instance();

    // Compute the maximum state value in the sm so we know how big
    // to make the table
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;
    BOOST_STATIC_CONSTANT(int, max_state = (mp11::mp_size<state_set_mp11>::value));
    state_dispatch_table m_state_dispatch_tables[max_state+1];
};

}}} // boost::msm::backmp11

#endif //BOOST_MSM_BACKMP11_FAVOR_COMPILE_TIME_H
