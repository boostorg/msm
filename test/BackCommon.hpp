// Copyright 2024 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// back-end
#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/backmp11/favor_compile_time.hpp>
#include <boost/msm/back11/state_machine.hpp>

template<typename Front>
using get_test_machines = boost::mpl::vector<
    boost::msm::back::state_machine<Front>,
    boost::msm::back::state_machine<Front, boost::msm::back::favor_compile_time>>;
    // boost::msm::back11::state_machine<Front>
    // >;

#define BOOST_MSM_TEST_DEFINE_DEPENDENT_TEMPLATES(frontname)                          \
    using base = msm::front::state_machine_def<frontname>;                            \
    template<typename T1, class Event, typename T2>                                   \
    using _row = typename base::template _row<T1, Event, T2>;                         \
    template<                                                                         \
        typename T1,                                                                  \
        class Event,                                                                  \
        typename T2,                                                                  \
        void (frontname::*action)(Event const&),                                      \
        bool (frontname::*guard)(Event const&)                                        \
        > using row = typename base::template row<T1, Event, T2, action, guard>;      \
    template<                                                                         \
        typename T1,                                                                  \
        class Event,                                                                  \
        typename T2,                                                                  \
        void (frontname::*action)(Event const&)                                       \
        > using a_row = typename base::template a_row<T1, Event, T2, action>;         \
    template<                                                                         \
        typename T1,                                                                  \
        class Event,                                                                  \
        typename T2,                                                                  \
        bool (frontname::*guard)(Event const&)                                        \
    > using g_row = typename base::template g_row<T1, Event, T2, guard>;



// template<typename State, typename Fsm>
// State& get_state(Fsm&& fsm)
// {
//     return fsm.template get_state<State&>();
// }