//Copyright Florian Goujeon 2021.
//Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE or copy at
//https://www.boost.org/LICENSE_1_0.txt)
//Official repository: https://github.com/fgoujeon/fsm-benchmark

// #include "common.hpp"
#define PROBLEM_SIZE 10
#define PROBLEM_SIZE_X_2 20

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE PROBLEM_SIZE_X_2
#define BOOST_MPL_LIMIT_MAP_SIZE PROBLEM_SIZE_X_2

#define COUNTER \
    X(0) \
    X(1) \
    X(2) \
    X(3) \
    X(4) \
    X(5) \
    X(6) \
    X(7) \
    X(8) \
    X(9)
    // X(10) \
    // X(11) \
    // X(12) \
    // X(13) \
    // X(14) \
    // X(15) \
    // X(16) \
    // X(17) \
    // X(18) \
    // X(19) \
    // X(20) \
    // X(21) \
    // X(22) \
    // X(23) \
    // X(24)

#define COMMA_0
#define COMMA_1  ,
#define COMMA_2  ,
#define COMMA_3  ,
#define COMMA_4  ,
#define COMMA_5  ,
#define COMMA_6  ,
#define COMMA_7  ,
#define COMMA_8  ,
#define COMMA_9  ,
#define COMMA_10 ,
#define COMMA_11 ,
#define COMMA_12 ,
#define COMMA_13 ,
#define COMMA_14 ,
#define COMMA_15 ,
#define COMMA_16 ,
#define COMMA_17 ,
#define COMMA_18 ,
#define COMMA_19 ,
#define COMMA_20 ,
#define COMMA_21 ,
#define COMMA_22 ,
#define COMMA_23 ,
#define COMMA_24 ,

#define COMMA_IF_NOT_0(N) COMMA_##N

constexpr auto test_loop_size = 1000;

static int test();

// int main()
// {
//     constexpr auto main_loop_size = 1000;

//     auto counter = 0;

//     for(auto i = 0; i < main_loop_size; ++i)
//     {
//         counter += test();
//     }

//     constexpr auto expected_counter = test_loop_size * main_loop_size * PROBLEM_SIZE;

//     if(counter != expected_counter)
//     {
//         return 1;
//     }

//     return 0;
// }




#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

template<int Index>
struct state_transition_event
{
    int two = 2;
};

struct internal_transition_event
{
    int two = 2;
};

template<int Index>
struct state_tpl: msm::front::state<>
{
    template<class Event, class Fsm>
    void on_exit(const Event& event, Fsm& fsm)
    {
        fsm.process_event(internal_transition_event{});
    }
};

template<int Index>
struct state_transition_action
{
    template<class Event, class Fsm, class SourceState, class TargetState>
    void operator()(const Event& evt, Fsm& sm, SourceState&, TargetState&)
    {
        sm.counter = (sm.counter + 1) * evt.two;
    }
};

template<int Index>
struct internal_transition_action
{
    template<class Event, class Fsm, class SourceState, class TargetState>
    void operator()(const Event& evt, Fsm& sm, SourceState&, TargetState&)
    {
        sm.counter /= evt.two;
    }
};

template<int Index>
struct guard
{
    template<class Event, class Fsm, class SourceState, class TargetState>
    bool operator()(const Event& evt, Fsm& sm, SourceState&, TargetState&)
    {
        return evt.two >= 0;
    }
};

struct fsm_: public msm::front::state_machine_def<fsm_>
{
    using initial_state = state_tpl<0>;

    struct transition_table: mpl::vector
    <
#define X(N) \
        COMMA_IF_NOT_0(N) Row<state_tpl<N>, state_transition_event<N>, state_tpl<(N + 1) % PROBLEM_SIZE>, state_transition_action<N>, guard<N>> \
        , Row<state_tpl<N>, internal_transition_event, none, internal_transition_action<N>>
        COUNTER
#undef X
    >{};

    int counter = 0;
};

using fsm = msm::back::state_machine<fsm_>;

static int test()
{
    auto sm = fsm{};

    sm.start();

    for(auto i = 0; i < test_loop_size; ++i)
    {
#define X(N) \
    sm.process_event(state_transition_event<N>{});
        COUNTER
#undef X
    }

    return sm.counter;
}