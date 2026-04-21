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

#ifndef BOOST_MSM_BACKMP11_DETAIL_HISTORY_IMPL_HPP
#define BOOST_MSM_BACKMP11_DETAIL_HISTORY_IMPL_HPP

#include <boost/msm/backmp11/detail/metafunctions.hpp>
#include <boost/msm/front/history_policies.hpp>

namespace boost::msm::backmp11::detail
{

template <typename History, typename InitialStateIds>
class history_impl;

template <typename InitialStateIds>
class history_impl<front::no_history, InitialStateIds>
{
  public:
    template <typename StateMachine, typename Event>
    void on_entry(StateMachine& sm, const Event&)
    {
        sm.m_active_state_ids = value_array<InitialStateIds>;
        if constexpr (StateMachine::event_pool_member::value)
        {
            sm.get_event_pool().events.clear();
        }
    }

    template <typename StateMachine, typename Event, typename Visitor>
    void on_entry(StateMachine& sm, const Event& event, Visitor&& visitor)
    {
        // First set all active state ids...
        on_entry(sm, event);
        // ... then execute each state entry.
        mp11::mp_for_each<InitialStateIds>(
            [&sm, &visitor](auto state_id)
            {
                auto& state = std::get<decltype(state_id)::value>(sm.m_states);
                visitor(state);
            });
    }

    template <typename StateMachine>
    void on_exit(StateMachine&)
    {
    }

  private:
    // Allow access to private members for serialization.
    template<typename T, typename U>
    friend void serialize(T&, history_impl<front::no_history, U>&);
};

template <typename InitialStateIds>
class history_impl<front::always_shallow_history, InitialStateIds>
{
public:
    template <typename StateMachine, typename Event>
    void on_entry(StateMachine& sm, const Event&)
    {
        sm.m_active_state_ids = m_last_active_state_ids;
    }

    template <typename StateMachine, typename Event, typename Visitor>
    void on_entry(StateMachine& sm, const Event& event, Visitor&& visitor)
    {
        // First set all active state ids...
        on_entry(sm, event);
        // ... then execute each state entry.
        sm.template visit<visit_mode::active_non_recursive>(visitor);
    }

    template <typename StateMachine>
    void on_exit(StateMachine& sm)
    {
        m_last_active_state_ids = sm.m_active_state_ids;
    }

  private:
    // Allow access to private members for serialization.
    template<typename T, typename U>
    friend void serialize(T&, history_impl<front::always_shallow_history, U>&);

    std::array<uint16_t, mp11::mp_size<InitialStateIds>::value>
        m_last_active_state_ids{value_array<InitialStateIds>};
};

template <typename... Events, typename InitialStateIds>
class history_impl<front::shallow_history<Events...>, InitialStateIds>
{
    using events = mp11::mp_list<Events...>;

public:
    template <typename StateMachine, typename Event>
    void on_entry(StateMachine& sm, const Event&)
    {
        if constexpr (mp11::mp_contains<events, Event>::value)
        {
            sm.m_active_state_ids = m_last_active_state_ids;
        }
        else
        {
            sm.m_active_state_ids = value_array<InitialStateIds>;
            if constexpr (StateMachine::event_pool_member::value)
            {
                sm.get_event_pool().events.clear();
            }
        }
    }

    template <typename StateMachine, typename Event, typename Visitor>
    void on_entry(StateMachine& sm, const Event& event, Visitor&& visitor)
    {
        // First set all active state ids...
        on_entry(sm, event);
        // ... then execute each state entry.
        sm.template visit<visit_mode::active_non_recursive>(visitor);
    }

    template <typename StateMachine>
    void on_exit(StateMachine& sm)
    {
        m_last_active_state_ids = sm.m_active_state_ids;
    }

  private:
    // Allow access to private members for serialization.
    template<typename T, typename... Es, typename U>
    friend void serialize(T&, history_impl<front::shallow_history<Es...>, U>&);

    std::array<uint16_t, mp11::mp_size<InitialStateIds>::value>
        m_last_active_state_ids{value_array<InitialStateIds>};
};

} // boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_DETAIL_HISTORY_IMPL_HPP
