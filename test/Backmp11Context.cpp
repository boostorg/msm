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

#include <boost/msm/back/queue_container_circular.hpp>
#include <boost/msm/back/history_policies.hpp>

#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE backmp11_context_test
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

    // Context.
    struct Context
    {
        uint32_t machine_entries = 0;
        uint32_t machine_exits = 0;
    };
    template<typename Fsm>
    static Context& get_context(Fsm& fsm)
    {
        return fsm.get_context();
    }

    // States.
    struct Default : public state<>{};

    // Forward-declare the upper machine,
    // so we can set a root_sm.
    struct UpperMachine;

    struct SmConfig : state_machine_config
    {
        using context = Context;
        using root_sm = UpperMachine;
    };

    template<typename T>
    struct MachineBase_ : public state_machine_def<MachineBase_<T>>
    {
        template <typename Event, typename Fsm>
        void on_entry(const Event& /*event*/, Fsm& fsm)
        {
            Context& context = get_context(fsm);
            context.machine_entries++;
        };

        template <typename Event, typename Fsm>
        void on_exit(const Event& /*event*/, Fsm& fsm)
        {
            get_context(fsm).machine_exits++;
        };

        using initial_state = Default;
    };

    struct LowerMachine_ : public MachineBase_<LowerMachine_> {};
    using LowerMachine = state_machine<LowerMachine_, SmConfig>;

    struct MiddleMachine_ : public MachineBase_<MiddleMachine_>
    {
        using transition_table = mp11::mp_list<
            Row< Default      , EnterSubFsm , LowerMachine >,
            Row< LowerMachine , ExitSubFsm  , Default      >
        >;
    };
    using MiddleMachine = state_machine<MiddleMachine_, SmConfig>;

    struct UpperMachine_ : public MachineBase_<UpperMachine_>
    {
        using transition_table = mp11::mp_list<
            Row< Default       , EnterSubFsm , MiddleMachine>,
            Row< MiddleMachine , ExitSubFsm  , Default>
        >;
    };
    class UpperMachine : public state_machine<UpperMachine_, SmConfig, UpperMachine>
    {
      public:
        using Base = state_machine<UpperMachine_, SmConfig, UpperMachine>;
        using Base::Base;
    };

    BOOST_AUTO_TEST_CASE( backmp11_context_test )
    {
        Context context;
        UpperMachine test_machine{context};

        test_machine.start(); 
        BOOST_CHECK_MESSAGE(test_machine.get_context().machine_entries == 1, "SM entry not called correctly");

        test_machine.process_event(EnterSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.get_context().machine_entries == 2, "SM entry not called correctly");

        test_machine.process_event(EnterSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.get_context().machine_entries == 3, "SM entry not called correctly");

        test_machine.process_event(ExitSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.get_context().machine_exits == 1, "SM exit not called correctly");

        test_machine.process_event(ExitSubFsm()); 
        BOOST_CHECK_MESSAGE(test_machine.get_context().machine_exits == 2, "SM exit not called correctly");

        test_machine.stop(); 
        BOOST_CHECK_MESSAGE(test_machine.get_context().machine_exits == 3, "SM exit not called correctly");
    }
}

