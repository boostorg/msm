//Copyright Florian Goujeon 2021.
//Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE or copy at
//https://www.boost.org/LICENSE_1_0.txt)
//Official repository: https://github.com/fgoujeon/fsm-benchmark

#define PROBLEM_SIZE 25
#define PROBLEM_SIZE_X_2 50

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
    X(9) \
    X(10) \
    X(11) \
    X(12) \
    X(13) \
    X(14) \
    X(15) \
    X(16) \
    X(17) \
    X(18) \
    X(19) \
    X(20) \
    X(21) \
    X(22) \
    X(23) \
    X(24)

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

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE PROBLEM_SIZE_X_2
#define BOOST_MPL_LIMIT_MAP_SIZE PROBLEM_SIZE_X_2
#define BOOST_MPL_LIMIT_SET_SIZE PROBLEM_SIZE_X_2

#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <iostream>

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

struct enter_sub_fsm_event
{
    int two = 2;
};

struct exit_sub_fsm_event
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

    // Don't trigger an additional event processing when we exit a fsm, forces
    // the dispatch tables of the sub-fsms to be created in all CUs.
    template<class Fsm>
    void on_exit(const exit_sub_fsm_event& event, Fsm& fsm)
    {
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

template<size_t Offset = 0, typename SubFsm = void>
struct fsm_: public msm::front::state_machine_def<fsm_<Offset, SubFsm>>
{
    using initial_state = state_tpl<0+Offset>;
    using sub_fsm = SubFsm;

    using transition_table = TRANSITION_TABLE_TYPE
    <
#define X(N) \
        COMMA_IF_NOT_0(N) Row<state_tpl<N+Offset>, state_transition_event<N+Offset>, state_tpl<(N + 1) % PROBLEM_SIZE+Offset>, state_transition_action<N>, guard<N>> \
        , Row<state_tpl<N+Offset>, internal_transition_event, none, internal_transition_action<N>>
        COUNTER
#undef X
    , Row<state_tpl<Offset>, enter_sub_fsm_event, sub_fsm, none, none>
    , Row<sub_fsm, exit_sub_fsm_event, state_tpl<Offset>, none, none>
    >;

    int counter = 0;
    static constexpr size_t offset = Offset;
};

template<size_t Offset>
struct fsm_<Offset, void>: public msm::front::state_machine_def<fsm_<Offset, void>>
{
    using initial_state = state_tpl<0+Offset>;

    using transition_table = TRANSITION_TABLE_TYPE
    <
#define X(N) \
        COMMA_IF_NOT_0(N) Row<state_tpl<N+Offset>, state_transition_event<N+Offset>, state_tpl<(N + 1) % PROBLEM_SIZE+Offset>, state_transition_action<N>, guard<N>> \
        , Row<state_tpl<N+Offset>, internal_transition_event, none, internal_transition_action<N>>
        COUNTER
#undef X
    >;

    int counter = 0;
    static constexpr size_t offset = Offset;
};

template<typename Fsm>
int run_fsm()
{
    auto first_sm = Fsm{};
    auto& second_sm = first_sm.template get_state<typename Fsm::sub_fsm&>();
    auto& third_sm = second_sm.template get_state<typename Fsm::sub_fsm::sub_fsm&>();

    first_sm.start();

    // First FSM
    for(auto i = 0; i < test_loop_size; ++i)
    {
#define X(N) \
    first_sm.process_event(state_transition_event<N>{});
        COUNTER
#undef X
    }

    // Second FSM
    first_sm.process_event(enter_sub_fsm_event{});
    for(auto i = 0; i < test_loop_size; ++i)
    {
#define X(N) \
    first_sm.process_event(state_transition_event<N+Fsm::sub_fsm::offset>{});
        COUNTER
#undef X
    }

    // Third FSM
    first_sm.process_event(enter_sub_fsm_event{});
    for(auto i = 0; i < test_loop_size; ++i)
    {
#define X(N) \
    first_sm.process_event(state_transition_event<N+Fsm::sub_fsm::sub_fsm::offset>{});
        COUNTER
#undef X
    }

    auto sum = first_sm.counter + second_sm.counter + third_sm.counter;

    first_sm.process_event(exit_sub_fsm_event{});
    first_sm.process_event(exit_sub_fsm_event{});

    return sum;
}

template<typename Fsm>
void test_fsm()
{
    constexpr auto main_loop_size = 1000;

    auto counter = 0;

    for(auto i = 0; i < main_loop_size; ++i)
    {
        counter += run_fsm<Fsm>();
    }

    constexpr auto expected_counter = test_loop_size * main_loop_size * PROBLEM_SIZE * 3;

    std::cout << counter << std::endl;
    std::cout << expected_counter << std::endl;

    assert(counter == expected_counter);
}