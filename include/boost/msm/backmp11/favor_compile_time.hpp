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

template <class Fsm>
struct process_any_event_helper
{
    process_any_event_helper(msm::back::HandledEnum& res_,Fsm* self_,::boost::any any_event_):
    res(res_),self(self_),any_event(any_event_),finished(false){}
    
    static HandledEnum execute(Fsm* self, ::boost::any const& any_event)
    {
        typedef typename recursive_get_transition_table<Fsm>::type stt;
        typedef typename generate_event_set<stt>::event_set_mp11 stt_events;
        typedef typename recursive_get_internal_transition_table<Fsm, ::boost::mpl::true_>::type istt;
        typedef typename generate_event_set<typename Fsm::template create_real_stt<Fsm, istt>::type>::event_set_mp11 istt_events;
        typedef mp11::mp_set_union<stt_events, istt_events> all_events;
        
        HandledEnum res= HANDLED_FALSE;
        mp11::mp_for_each<mp11::mp_transform<mp11::mp_identity, all_events>>(process_any_event_helper<Fsm>(res, self, any_event));
        return res;
    }
    
    template <class Event>
    void operator()(mp11::mp_identity<Event> const&)
    {
        if (!finished && ::boost::any_cast<Event>(&any_event) != 0)
        {
            finished = true;
            res = self->process_event_internal(::boost::any_cast<Event>(any_event));
        }
    }
private:
    msm::back::HandledEnum&     res;
    Fsm*                        self;
    ::boost::any                any_event;
    bool                        finished;
};

#define BOOST_MSM_BACKMP11_GENERATE_PROCESS_EVENT(fsmname)                                          \
    template<>                                                                                      \
    ::boost::msm::back::HandledEnum fsmname::process_any_event( ::boost::any const& any_event)      \
    {                                                                                               \
        return ::boost::msm::back::process_any_event_helper<fsmname>::execute(this, any_event);     \
    }                                                                                               \
    template<>                                                                                      \
    template<class Event, class Policy>                                                             \
    typename ::boost::enable_if<                                                                    \
        boost::is_same<Policy, boost::msm::back::favor_compile_time>,                               \
        boost::msm::back::execute_return>::type                                                     \
    BOOST_NOINLINE fsmname::process_event_internal(Event const& evt, EventSource source)            \
    {                                                                                               \
        return process_event_internal_impl(evt, source);                                            \
    }

#define BOOST_MSM_BACKMP11_GENERATE_FSM(fsmname)       \
    BOOST_MSM_BACKMP11_GENERATE_PROCESS_EVENT(fsmname)


struct favor_compile_time
{
    typedef int compile_policy;
    typedef ::boost::mpl::false_ add_forwarding_rows;
};

struct chain_row
{
    template<typename Fsm, typename Event>
    HandledEnum operator()(Fsm& fsm, int region,int state,Event const& evt) const
    {
        typedef HandledEnum (*real_cell)(Fsm&, int,int,Event const&);
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
    std::deque<generic_cell> one_state;
};

template<>
struct cell_initializer<favor_compile_time>
{
    static void init(chain_row* entries, const generic_init_cell_value* array, size_t size)
    {
        for (size_t i=0; i<size; i++)
        {
            const auto& item = array[i];
            entries[item.index].one_state.push_front(item.address);
        }
    }
};

// split the stt into rows grouped by state
template<typename Stt>
struct group_rows_by_state
{
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;
    // Consider only real rows
    template<typename Transition>
    using is_real_row = mp11::mp_not<typename has_not_real_row_tag<Transition>::type>;
    typedef mp11::mp_copy_if<Stt, is_real_row> real_rows;
    // Go through each row and group transitions by state.
    template<typename Row, typename State>
    using has_state = std::is_same<typename Row::current_state_type, State>;
    template<typename V, typename State>
    using partition_by_state = mp11::mp_list<
        // From first element we remove the rows that we have grouped
        mp11::mp_remove_if_q<
            mp11::mp_first<V>,
            mp11::mp_bind_back<has_state, State>
            >,
        // To second element we append a group of rows that is handled by the state
        mp11::mp_push_back<
            mp11::mp_second<V>,
            mp11::mp_copy_if_q<
                mp11::mp_first<V>,
                mp11::mp_bind_back<has_state, State>
                >
            >
        >;
    typedef mp11::mp_fold<
        state_set_mp11,
        mp11::mp_list<real_rows, mp11::mp_list<>>,
        partition_by_state
        > grouped_rows;
    // static_assert(mp11::mp_empty<mp11::mp_first<grouped_rows>>::value, "Internal error: Not all rows grouped");
    typedef mp11::mp_second<grouped_rows> type;
};

template<typename Stt, typename State>
using get_rows_of_state = mp11::mp_at_c<
    typename group_rows_by_state<Stt>::type,
    get_state_id<Stt, State>::value
    >;

// Convert an event to a type index.
template<class Event>
inline std::type_index to_type_index()
{
    return std::type_index{typeid(Event)};
}
template<class Event>
inline std::type_index to_type_index(const Event&)
{
    return std::type_index{typeid(Event)};
}

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the SM take its transition on the given
// Event type.
template<class Fsm, class Stt>
struct dispatch_table<Fsm, Stt, back::favor_compile_time>
{
public:
    // Dispatch function for a specific event.
    template<class Event>
    using cell = HandledEnum (*)(Fsm&, int,int,Event const&);

    // Dispatch an event.
    template<class Event>
    static HandledEnum dispatch(Fsm& fsm, int region_id, int state_id, const Event& event)
    {
        return instance().m_state_dispatch_tables[state_id+1].dispatch(fsm, region_id, state_id, event);
    }

    // Dispatch an event to the FSM's internal table.
    template<class Event>
    static HandledEnum dispatch_internal(Fsm& fsm, int region_id, int state_id, const Event& event)
    {
        return instance().m_state_dispatch_tables[0].dispatch(fsm, region_id, state_id, event);
    }

private:
    dispatch_table()
    {
        // TODO:
        // Initialize internal dispatch table.

        mp11::mp_for_each<state_set_mp11>(
            [&](auto state)
            {
                using State = decltype(state);
                m_state_dispatch_tables[get_state_id<Stt, State>::value+1].template init<State>();
            }
        );
    }

    // The singleton instance.
    static const dispatch_table& instance()
    {
        static dispatch_table table;
        return table;
    }

    // Dispatch table for one state.
    class state_dispatch_table
    {
    public:
        // Initialize the table for the given state.
        template<typename State>
        void init()
        {
            // Fill in cells for all rows of the stt.
            mp11::mp_for_each<get_rows_of_state<Stt, State>>(
                [&](auto row)
                {
                    using Row = decltype(row);
                    auto& chain_row = m_entries[to_type_index<typename Row::transition_event>()];
                    chain_row.one_state.push_front(reinterpret_cast<generic_cell>(&Row::execute));
                }
            );

            // Fill in cells for deferred events.
            mp11::mp_for_each<typename to_mp_list<typename State::deferred_events>::type>(
                [&](auto event)
                {
                    using Event = decltype(event);
                    auto& chain_row = m_entries[to_type_index<Event>()];
                    chain_row.one_state.push_front(reinterpret_cast<generic_cell>(&Fsm::template defer_transition<Event>));
                }
            );

            // TODO:
            // Fill in cells for calling a submachine.
            // We also need to make sure conflicting transitions are handled correctly.
            if constexpr (is_composite_state<State>::value)
            {
                m_call_submachine = [](Fsm& fsm, const boost::any& evt)
                {
                    return (fsm.template get_state<State&>()).process_any_event(evt);
                };
            }
        }

        // Dispatch an event.
        template<class Event>
        HandledEnum dispatch(Fsm& fsm, int region_id, int state_id, const Event& event) const
        {
            // TODO: Incorporate calling the submachine.
            auto it = m_entries.find(to_type_index(event));
            if (it != m_entries.end())
            {
                return (it->second)(fsm, region_id, state_id, event);
            }
            return HANDLED_FALSE;
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
