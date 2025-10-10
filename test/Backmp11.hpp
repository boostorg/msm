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

#include "boost/msm/backmp11/state_machine_config.hpp"
#include "boost/msm/backmp11/favor_compile_time.hpp"
#include "boost/msm/backmp11/state_machine.hpp"
#include <boost/msm/back/queue_container_deque.hpp>
#define BOOST_PARAMETER_CAN_USE_MP11
#include <boost/parameter.hpp>

namespace boost::msm::backmp11
{

struct favor_compile_time_config : boost::msm::backmp11::state_machine_config
{
    using compile_policy = boost::msm::backmp11::favor_compile_time;
};

using back::queue_container_deque;

BOOST_PARAMETER_TEMPLATE_KEYWORD(front_end)
BOOST_PARAMETER_TEMPLATE_KEYWORD(history_policy)
BOOST_PARAMETER_TEMPLATE_KEYWORD(compile_policy)
BOOST_PARAMETER_TEMPLATE_KEYWORD(fsm_check_policy)
BOOST_PARAMETER_TEMPLATE_KEYWORD(queue_container_policy)

template <
    class A1,
    class A2,
    class A3,
    class A4
>
struct state_machine_config_adapter : state_machine_config
{
    typedef ::boost::parameter::parameters<
      ::boost::parameter::optional<
            ::boost::parameter::deduced< tag::history_policy>, has_history_policy< ::boost::mpl::_ >
        >
    , ::boost::parameter::optional<
            ::boost::parameter::deduced< tag::compile_policy>, has_compile_policy< ::boost::mpl::_ >
        >
    , ::boost::parameter::optional<
            ::boost::parameter::deduced< tag::fsm_check_policy>, has_fsm_check< ::boost::mpl::_ >
        >
    , ::boost::parameter::optional<
            ::boost::parameter::deduced< tag::queue_container_policy>,
            has_queue_container_policy< ::boost::mpl::_ >
        >
    > config_signature;

    typedef typename
        config_signature::bind<A1,A2,A3,A4>::type
        config_args;

    typedef typename ::boost::parameter::binding<
        config_args, tag::history_policy, NoHistory >::type              history;

    typedef typename ::boost::parameter::binding<
        config_args, tag::compile_policy, favor_runtime_speed >::type    compile_policy;

    typedef typename ::boost::parameter::binding<
        config_args, tag::queue_container_policy,
        queue_container_deque >::type                                    QueueContainerPolicy;
    template<typename T>
    using queue_container = typename QueueContainerPolicy::template In<T>::type;
};

// TODO:
// Doesn't work, would also fail with inheritance in state_machine.
// template <class A0,
//           class A1 = parameter::void_,
//           class A2 = parameter::void_,
//           class A3 = parameter::void_,
//           class A4 = parameter::void_>
// class state_machine_adapter
//     : public state_machine<A0, state_machine_config_adapter<A1, A2, A3, A4>>
// {
//     using Base = state_machine<A0, state_machine_config_adapter<A1, A2, A3, A4>>;
//   public:
//     using Base::Base;
// };

template <class A0,
          class A1 = parameter::void_,
          class A2 = parameter::void_,
          class A3 = parameter::void_,
          class A4 = parameter::void_>
using state_machine_adapter = state_machine<A0, state_machine_config_adapter<A1, A2, A3, A4>>;

} // boost::msm::backmp11