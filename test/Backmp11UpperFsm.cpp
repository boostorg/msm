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
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>

#include <boost/msm/back/queue_container_circular.hpp>
#include <boost/msm/back/history_policies.hpp>

#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE backmp11_upper_fsm_test
#endif
#include <boost/test/unit_test.hpp>

namespace msm = boost::msm;
namespace mp11 = boost::mp11;

using namespace msm::front;
using namespace msm::backmp11;

namespace
{
    // Events.
    struct EnterSubFsm{};
    struct ExitSubFsm{};

    // States.
    struct Default : public state<>{};

    // Forward-declare the backend of the upper machine,
    // so we can use it as Context.
    struct UpperMachine;
    using Context = UpperMachine;

    template<typename T>
    struct MachineBase_ : public state_machine_def<MachineBase_<T>>
    {
        template <typename Event, typename Fsm>
        void on_entry(const Event& /*event*/, Fsm& fsm)
        {
            fsm.get_context()->machine_entries++;
        };

        template <typename Event, typename Fsm>
        void on_exit(const Event& /*event*/, Fsm& fsm)
        {
            fsm.get_context()->machine_exits++;
        };

        using initial_state = Default;
    };

    struct LowerMachine_ : public MachineBase_<LowerMachine_> {};
    using LowerMachine = state_machine<LowerMachine_, use_context<Context>>;

    struct MiddleMachine_ : public MachineBase_<MiddleMachine_>
    {
        using transition_table = mp11::mp_list<
            Row< Default      , EnterSubFsm , LowerMachine >,
            Row< LowerMachine , ExitSubFsm  , Default      >
        >;
    };
    using MiddleMachine = state_machine<MiddleMachine_, use_context<Context>>;

    struct UpperMachine_ : public MachineBase_<UpperMachine_>
    {
        using transition_table = mp11::mp_list<
            Row< Default       , EnterSubFsm , MiddleMachine>,
            Row< MiddleMachine , ExitSubFsm  , Default>
        >;

        uint32_t machine_entries = 0;
        uint32_t machine_exits = 0;
    };
    struct UpperMachine : state_machine<UpperMachine_, use_context<Context>> {};


    BOOST_AUTO_TEST_CASE( backmp11_upper_fsm_test )
    {
        UpperMachine test_machine{&test_machine};

        test_machine.start(); 
        BOOST_CHECK_MESSAGE(test_machine.machine_entries == 1, "SM entry not called correctly");

        test_machine.process_event(EnterSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.machine_entries == 2, "SM entry not called correctly");

        test_machine.process_event(EnterSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.machine_entries == 3, "SM entry not called correctly");

        test_machine.process_event(ExitSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.machine_exits == 1, "SM exit not called correctly");

        test_machine.process_event(ExitSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.machine_exits == 2, "SM exit not called correctly");

        test_machine.stop(); 
        BOOST_CHECK_MESSAGE(test_machine.machine_exits == 3, "SM exit not called correctly");
    }
}

