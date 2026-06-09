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
#include "BackCommon.hpp"
//front-end
#include "FrontCommon.hpp"
#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE test_constructor
#endif
#include <boost/test/unit_test.hpp>
#include <boost/config.hpp>

namespace msm = boost::msm;

using namespace msm::front;
using namespace msm::backmp11;


namespace
{

struct MyState : test::StateBase {};

struct StateMachine_ : test::StateMachineBase_<StateMachine_>
{
    using initial_state = MyState;

    StateMachine_() = default;
    
    StateMachine_(int n) : number(n) {}

    int number{};
};

struct Context
{
    int number{};
};

struct context_config : state_machine_config
{
    using context = Context;
};

BOOST_AUTO_TEST_CASE(context_constructors)
{
    using StateMachine = state_machine<StateMachine_, context_config>;
    Context context;

    StateMachine sm{context};
}

} // namespace
