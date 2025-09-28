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

#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE backmp11_visitor_test
#endif
#include <boost/test/unit_test.hpp>

namespace msm = boost::msm;
namespace mp11 = boost::mp11;

using namespace msm::front;
using namespace msm::backmp11;

namespace
{
    struct StateBase
    {
        size_t visits;
    };

    class Visitor
    {
      public:
        virtual void visit(StateBase& state, int& visits) = 0;
    };

    struct CountVisitor : public Visitor
    {
        void visit(StateBase& state, int& visits) override
        {
            visits += 1;
            state.visits += 1;
        }
    };

    struct ResetCountVisitor : public Visitor
    {
        void visit(StateBase& state, int& visits) override
        {
            visits = 0;
            state.visits = 0;
        }
    };

    // base state for all states of the fsm, to make them visitable
    struct VisitableState : StateBase
    {
        // implementation of the accept function
        void accept(Visitor& vis, int& i)
        {
            vis.visit(*this, i);
        }
    };

    // Events.
    struct EnterSubFsm{};
    struct ExitSubFsm{};

    // States.
    struct DefaultState : public state<VisitableState>{ };

    template<typename T>
    struct MachineBase_ : public state_machine_def<MachineBase_<T>, VisitableState>
    {
        using initial_state = DefaultState;
    };

    struct LowerMachine_ : public MachineBase_<LowerMachine_> { };
    using LowerMachine = state_machine<LowerMachine_>;

    struct MiddleMachine_ : public MachineBase_<MiddleMachine_>
    {
        using transition_table = mp11::mp_list<
            Row< DefaultState , EnterSubFsm , LowerMachine >,
            Row< LowerMachine , ExitSubFsm  , DefaultState >
        >;
    };
    using MiddleMachine = state_machine<MiddleMachine_>;

    struct UpperMachine_ : public MachineBase_<UpperMachine_>
    {
        using transition_table = mp11::mp_list<
            Row< DefaultState  , EnterSubFsm , MiddleMachine>,
            Row< MiddleMachine , ExitSubFsm  , DefaultState>
        >;
    };
    using UpperMachine = state_machine<UpperMachine_>;
    
    BOOST_AUTO_TEST_CASE( backmp11_upper_fsm_test )
    {
        UpperMachine upper_machine{};
        CountVisitor count_visitor;
        ResetCountVisitor reset_count_visitor;
        int visits = 0;
        auto& upper_machine_state = upper_machine.get_state<DefaultState&>();
        auto& middle_machine = upper_machine.get_state<MiddleMachine&>();
        auto& middle_machine_state = middle_machine.get_state<DefaultState&>();
        auto& lower_machine = middle_machine.get_state<LowerMachine&>();
        auto& lower_machine_state = lower_machine.get_state<DefaultState&>();

        upper_machine.visit_current_states(count_visitor, visits);
        BOOST_REQUIRE(visits == 0);
        
        upper_machine.start();
        upper_machine.visit_current_states(count_visitor, visits);
        BOOST_REQUIRE(upper_machine_state.visits == 1);
        BOOST_REQUIRE(visits == 1);
        upper_machine.visit_current_states(reset_count_visitor, visits);

        upper_machine.process_event(EnterSubFsm());
        upper_machine.visit_current_states(count_visitor, visits);
        BOOST_REQUIRE(middle_machine.visits == 1);
        BOOST_REQUIRE(middle_machine_state.visits == 1);
        BOOST_REQUIRE(visits == 2);
        upper_machine.visit_current_states(reset_count_visitor, visits);

        upper_machine.process_event(EnterSubFsm());
        upper_machine.visit_current_states(count_visitor, visits);
        BOOST_REQUIRE(middle_machine.visits == 1);
        BOOST_REQUIRE(lower_machine.visits == 1);
        BOOST_REQUIRE(lower_machine_state.visits == 1);
        BOOST_REQUIRE(visits == 3);
        upper_machine.visit_current_states(reset_count_visitor, visits);

        upper_machine.stop();
        upper_machine.visit_current_states(count_visitor, visits);
        BOOST_REQUIRE(visits == 0);
    }
}

