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

#ifndef BOOST_MSM_BACKMP11_HISTORY_IMPL_H
#define BOOST_MSM_BACKMP11_HISTORY_IMPL_H

#include <boost/msm/front/history_policies.hpp>
#include <boost/mp11.hpp>

namespace boost { namespace msm { namespace backmp11
{

// Implementations for history policies.

template<typename History, int NumberOfRegions>
class history_impl;

template <int NumberOfRegions>
class history_impl<front::no_history, NumberOfRegions>
{
public:
    void set_initial_states(int* const initial_states)
    {
        for (int i=0;i<NumberOfRegions;++i)
            m_initialStates[i] = initial_states[i];
    }
    void history_exit(int* const )
    {
        // ignore
    }
    // returns the state where the state machine should be at start
    template <class Event>
    const int* history_entry(Event const& )
    {
        // always come back to the original state
        return m_initialStates;
    }
    history_impl& operator=(history_impl const& rhs)
    {
         for (int i=0; i<NumberOfRegions;++i)
         {
             m_initialStates[i] = rhs.m_initialStates[i];
         }
         return *this;
    }
    // this policy deletes all waiting deferred events
    template <class Event>
    bool process_deferred_events(Event const&)const
    {
        return false;
    }
    template<class Archive>
    void serialize(Archive & ar, const unsigned int)
    {
        ar & m_initialStates;
    }
private:
    int m_initialStates[NumberOfRegions];
};

template <int NumberOfRegions>
class history_impl<front::always_shallow_history, NumberOfRegions>
{
public:
    void set_initial_states(int* const initial_states)
    {
        for (int i=0;i<NumberOfRegions;++i)
            m_initialStates[i] = initial_states[i];
    }
    void history_exit(int* const current_states)
    {
        for (int i=0;i<NumberOfRegions;++i)
            m_initialStates[i] = current_states[i];
    }
    // returns the state where the state machine should be at start
    template <class Event>
    const int* history_entry(Event const& )
    {
        // always load back the last active state
        return m_initialStates;
    }
    history_impl& operator=(history_impl const& rhs)
    {
         for (int i=0; i<NumberOfRegions;++i)
         {
             m_initialStates[i] = rhs.m_initialStates[i];
         }
         return *this;
    }
    // the history policy keeps all deferred events until next reentry
    template <class Event>
    bool process_deferred_events(Event const&)const
    {
        return true;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int)
    {
        ar & m_initialStates;
    }
private:
    int m_initialStates[NumberOfRegions];
};

template <typename... Events, int NumberOfRegions>
class history_impl<front::shallow_history<Events...>, NumberOfRegions>
{
    using events_mp11 = mp11::mp_list<Events...>;

public:
    void set_initial_states(int* const initial_states)
    {
        for (int i=0;i<NumberOfRegions;++i)
        {
            m_currentStates[i] = initial_states[i];
            m_initialStates[i] = initial_states[i];
        }
    }
    void history_exit(int* const current_states)
    {
        for (int i=0;i<NumberOfRegions;++i)
            m_currentStates[i] = current_states[i];
    }
    // returns the state where the state machine should be at start
    template <class Event>
    const int* history_entry(Event const&)
    {
        if (mp11::mp_contains<events_mp11,Event>::value)
        {
            return m_currentStates;
        }
        // not one of our events, no history
        return m_initialStates;
    }
    history_impl& operator=(history_impl const& rhs)
    {
         for (int i=0; i<NumberOfRegions;++i)
         {
             m_initialStates[i] = rhs.m_initialStates[i];
             m_currentStates[i] = rhs.m_currentStates[i];
         }
         return *this;
    }
    // the history policy keeps deferred events until next reentry if coming from our history event
    template <class Event>
    bool process_deferred_events(Event const&)const
    {
        return mp11::mp_contains<events_mp11,Event>::value;
    }
    template<class Archive>
    void serialize(Archive & ar, const unsigned int)
    {
        ar & m_initialStates;
        ar & m_currentStates;
    }
private:
    int m_initialStates[NumberOfRegions];
    int m_currentStates[NumberOfRegions];
};

}}} // boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_HISTORY_IMPL_H
