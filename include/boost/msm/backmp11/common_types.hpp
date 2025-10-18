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

#ifndef BOOST_MSM_BACKMP11_COMMON_TYPES_H
#define BOOST_MSM_BACKMP11_COMMON_TYPES_H

#include <any>
#include <boost/msm/back/common_types.hpp>

namespace boost { namespace msm { namespace backmp11
{

using process_result = back::HandledEnum;

using any_event = std::any;

// Tag referring to active states of a SM.
struct active_states_t {};
constexpr active_states_t active_states;

// Tag referring to all states of a SM.
struct all_states_t {};
constexpr all_states_t all_states;

namespace detail
{
    using EventSource = back::EventSourceEnum;
    using back::HandledEnum;
}

}}} // namespace boost::msm::backmp11

#endif // BOOST_MSM_BACKMP11_COMMON_TYPES_H