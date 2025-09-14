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

#ifndef BOOST_MSM_BACKMP11_CONTEXT_H
#define BOOST_MSM_BACKMP11_CONTEXT_H

namespace boost { namespace msm { namespace backmp11
{

    template <typename T>
    struct use_context
    {
        using context_policy = int;
        using type = T;
    };

    struct no_context
    {
        using context_policy = int;
        using type = void;
    };

}}} // boost::msm::backmp11

#endif //BOOST_MSM_BACKMP11_CONTEXT_H
