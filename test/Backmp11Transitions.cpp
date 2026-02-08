// Copyright 2025 Christian Granzin
// Copyright 2010 Christophe Henry
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
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#include "Utils.hpp"

#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE backmp11_members_test
#endif
#include <boost/test/unit_test.hpp>

namespace msm = boost::msm;
namespace mp11 = boost::mp11;

using namespace msm::front;
using namespace msm::backmp11;

namespace
{

template <typename T>
struct Counter
{
    size_t count{};
};

// Events.
struct TriggerTransition{};
struct TriggerInternalTransition{};
struct TriggerSmInternalTransition{};
struct TriggerAnyTransition{};

// Actions
struct MyAction
{
    template<typename Event, typename Fsm, typename Source, typename Target>
    void operator()(const Event&, Fsm& fsm, Source&, Target&)
    {
        std::get<Counter<Source>>(fsm.action_calls).count++;
    }
};

// States.
struct MyState : public state<>{};
struct MyOtherState : public state<>{};
class StateMachine;
struct StateMachine_;

using Counters = std::tuple<
    Counter<MyState>,
    Counter<MyOtherState>,
    Counter<StateMachine_>,
    Counter<StateMachine>>;

struct SmConfig : state_machine_config
{
    using root_sm = StateMachine;
    // using compile_policy = favor_compile_time;
    template <typename T>
    using event_container = no_event_container<T>;
};

struct StateMachine_ : public state_machine_def<StateMachine_>
{
    using activate_deferred_events = int;

    template <typename Event, typename Fsm>
    void on_entry(const Event& /*event*/, Fsm& fsm)
    {
        fsm.entry_calls++;
    };

    template <typename Event, typename Fsm>
    void on_exit(const Event& /*event*/, Fsm& fsm)
    {
        fsm.exit_calls++;
    };

    using initial_state = MyState;
    using transition_table = mp11::mp_list<
        Row<MyState     , TriggerInternalTransition  , none        , MyAction, none>,
        Row<MyState     , TriggerSmInternalTransition, none        , MyAction, none>,
        Row<MyState     , TriggerTransition          , MyOtherState, MyAction, none>
    >;
    using internal_transition_table = mp11::mp_list<
        Internal<std::any                   , MyAction, none>,
        Internal<TriggerSmInternalTransition, MyAction, none>
    >;
};
// Leave this class without inheriting constructors to check
// that this only needs to be done in case we use a context
// (for convenience).
class StateMachine : public state_machine<StateMachine_, SmConfig, StateMachine>
{
  public:
    size_t entry_calls{};
    size_t exit_calls{};
    Counters action_calls{};
};

BOOST_AUTO_TEST_CASE( backmp11_members_test )
{
    StateMachine test_machine;

    test_machine.start();
    BOOST_REQUIRE(test_machine.entry_calls == 1);

    if constexpr (std::is_same_v<
                      typename StateMachine::config_t::compile_policy,
                      favor_runtime_speed>)
    {
        test_machine.process_event(TriggerAnyTransition{});
        ASSERT_ONE_AND_RESET(std::get<Counter<StateMachine>>(test_machine.action_calls).count);
    }

    test_machine.process_event(TriggerInternalTransition{});
    ASSERT_ONE_AND_RESET(std::get<Counter<MyState>>(test_machine.action_calls).count);

    // Ensure MyState consumes the event instead of StateMachine.
    test_machine.process_event(TriggerSmInternalTransition{});
    ASSERT_ONE_AND_RESET(std::get<Counter<MyState>>(test_machine.action_calls).count);

    test_machine.process_event(TriggerTransition{});
    ASSERT_ONE_AND_RESET(std::get<Counter<MyState>>(test_machine.action_calls).count);
    BOOST_REQUIRE(test_machine.is_state_active<MyOtherState>());

    if constexpr (std::is_same_v<
                      typename StateMachine::config_t::compile_policy,
                      favor_runtime_speed>)
    {
        test_machine.process_event(TriggerAnyTransition{});
        ASSERT_ONE_AND_RESET(std::get<Counter<StateMachine>>(test_machine.action_calls).count);
    }

    test_machine.process_event(TriggerSmInternalTransition{});
    ASSERT_ONE_AND_RESET(std::get<Counter<StateMachine>>(test_machine.action_calls).count);

    test_machine.stop();
    BOOST_REQUIRE(test_machine.exit_calls == 1);
}

} // namespace
