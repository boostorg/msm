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

#include <deque>
#include <typeindex>
#include <utility>

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

// TODO:
// Think about name BOOST_MSM_BACKMP11_GENERATE_DISPATCH_TABLE
#define BOOST_MSM_BACKMP11_GENERATE_FSM(fsmname)                                \
    template<>                                                                  \
    const fsmname::sm_dispatch_table& fsmname::sm_dispatch_table::instance()    \
    {                                                                           \
        static dispatch_table table;                                            \
        return table;                                                           \
    }
#define BOOST_MSM_BACKMP11_GENERATE_PROCESS_EVENT(fsmname) BOOST_MSM_BACKMP11_GENERATE_FSM(fsmname)

struct favor_compile_time
{
    typedef int compile_policy;
    typedef ::boost::mpl::false_ add_forwarding_rows;
};

struct chain_row
{
    template<typename Fsm>
    HandledEnum operator()(Fsm& fsm, int region, int state, any const& evt) const
    {
        typedef HandledEnum (*real_cell)(Fsm&, int, int, any const&);
        HandledEnum res = HANDLED_FALSE;
        typename std::deque<generic_cell>::const_iterator it = one_state.begin();
        while (it != one_state.end() && (res != HANDLED_TRUE && res != HANDLED_DEFERRED ))
        {
            auto fnc = reinterpret_cast<real_cell>(reinterpret_cast<void*>(*it));
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
    // TODO: Use any_cell.
    std::deque<generic_cell> one_state;
};


template<typename Stt>
struct get_real_rows
{
    template<typename Transition>
    using is_real_row = mp11::mp_not<typename has_not_real_row_tag<Transition>::type>;
    typedef mp11::mp_copy_if<Stt, is_real_row> type;
};
template<typename Stt, typename State>
struct get_rows_of_state
{
    template<typename Row>
    using has_state = std::is_same<typename Row::current_state_type, State>;    
    using type = mp11::mp_copy_if<
        typename get_real_rows<Stt>::type,
        has_state
        >;
};
template<typename Stt, typename State>
using get_rows_of_state_t = typename get_rows_of_state<Stt, State>::type;


// Convert an event to a type index.
template<class Event>
inline std::type_index to_type_index()
{
    return std::type_index{typeid(Event)};
}

// Adapter for calling a row's execute function.
template<typename Fsm, typename Event, typename Row>
HandledEnum convert_and_execute(Fsm& fsm, int region_id, int state_id, const any& event)
{
    return Row::execute(fsm, region_id, state_id, *any_cast<Event>(&event));
}

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the SM take its transition on the given
// Event type.
template<class Fsm, class Stt>
struct dispatch_table<Fsm, Stt, back::favor_compile_time>
{
public:
    // Dispatch an event.
    static HandledEnum dispatch(Fsm& fsm, int region_id, int state_id, const any& event)
    {
        return instance().m_state_dispatch_tables[state_id+1].dispatch(fsm, region_id, state_id, event);
    }

    // Dispatch an event to the FSM's internal table.
    static HandledEnum dispatch_internal(Fsm& fsm, int region_id, int state_id, const any& event)
    {
        return instance().m_state_dispatch_tables[0].dispatch(fsm, region_id, state_id, event);
    }

private:
    dispatch_table()
    {
        mp11::mp_for_each<state_set_mp11>(
            [&](auto state)
            {
                using State = decltype(state);
                m_state_dispatch_tables[get_state_id<Stt, State>::value+1].template init<State>();
            }
        );
    }

    // The singleton instance.
    static const dispatch_table& instance();

    // Dispatch table for one state.
    class state_dispatch_table
    {
    public:
        // Initialize the table for the given state.
        template<typename State>
        void init()
        {
            // Fill in cells for all rows of the stt.
            mp11::mp_for_each<get_rows_of_state_t<Stt, State>>(
                [&](auto row)
                {
                    using Row = decltype(row);
                    using Event = typename Row::transition_event;
                    auto& chain_row = m_entries[to_type_index<Event>()];
                    chain_row.one_state.push_front(reinterpret_cast<generic_cell>(&convert_and_execute<Fsm, Event, Row>));
                }
            );

            // Fill in cells for deferred events.
            mp11::mp_for_each<typename to_mp_list<typename State::deferred_events>::type>(
                [&](auto event)
                {
                    using Event = decltype(event);
                    auto& chain_row = m_entries[to_type_index<Event>()];
                    // TODO:
                    // Should also need some kind of conversion
                    chain_row.one_state.push_front(reinterpret_cast<generic_cell>(&Fsm::template defer_transition<Event>));
                }
            );

            // TODO:
            // We also need to make sure conflicting transitions are handled correctly.
            if constexpr (is_composite_state<State>::value)
            {
                m_call_submachine = [](Fsm& fsm, const any& evt)
                {
                    return (fsm.template get_state<State&>()).process_event_internal(evt);
                };
            }
        }

        // Dispatch an event.
        HandledEnum dispatch(Fsm& fsm, int region_id, int state_id, const any& event) const
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
        // Special member returned for entries that are not found in the table.
        chain_row m_default_chain_row;
        // Special member if the state is a composite
        std::function<HandledEnum(Fsm&, const boost::any&)> m_call_submachine;
    };

    // Compute the maximum state value in the sm so we know how big
    // to make the table
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;
    BOOST_STATIC_CONSTANT(int, max_state = (mp11::mp_size<state_set_mp11>::value));
    state_dispatch_table m_state_dispatch_tables[max_state+1];
};

}}} // boost::msm::back

#endif //BOOST_MSM_BACK_FAVOR_COMPILE_TIME_H
