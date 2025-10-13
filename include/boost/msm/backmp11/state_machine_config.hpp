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

#ifndef BOOST_MSM_BACKMP11_STATE_MACHINE_CONFIG_H
#define BOOST_MSM_BACKMP11_STATE_MACHINE_CONFIG_H

#include <deque>

#include <boost/msm/back/default_compile_policy.hpp>

namespace boost::msm::backmp11
{

using back::favor_runtime_speed;

// Config for the default context parameter
// (no context).
struct no_context {};

// Config for the default root sm parameter
// (no root sm).
struct no_root_sm {};

// Config for the default fsm parameter
// (processing fsm)
struct processing_sm{};

// Default state machine config.
struct default_state_machine_config
{
    using compile_policy = favor_runtime_speed;
    using context = no_context;
    using root_sm = no_root_sm;
    using fsm_parameter = processing_sm;
    template<typename T>
    using queue_container = std::deque<T>;
};

using state_machine_config = default_state_machine_config;

} // boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_STATE_MACHINE_CONFIG_H