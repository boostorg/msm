// Copyright 2011 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MSM_LPP_FUNCTION_H
#define BOOST_MSM_LPP_FUNCTION_H

#include <boost/preprocessor/tuple/elem.hpp> 
#include <boost/preprocessor/punctuation/comma_if.hpp> 
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>


namespace boost { namespace msm { namespace lpp
{

// user-defined functions
//todo variadic macro for number of arguments
#define BOOST_MSM_LPP_FUNCTION(funcname,userfunc)                                                                           \
template <class Param1=void , class Param2=void , class Param3=void , class Param4=void>                                    \
struct funcname ## impl : proto::extends< proto::terminal< boost::msm::lpp::tag::actor>::type,                             \
                          funcname ## impl<Param1,Param2,Param3,Param4> >{};                                                \
template <  > struct funcname ## impl< void,void,void,void>                                                                 \
    : proto::extends< proto::terminal<boost::msm::lpp::tag::actor>::type, funcname ## impl<void,void,void,void> >          \
{                                                                                                                           \
    template<class Sig> struct result;                                                                                      \
    template<class This,class B, class A0>                                                                                  \
    struct result<This(B& block,A0& a0)>                                                                                    \
    {                                                                                                                       \
        static A0& a0;                                                                                                      \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,userfunc(a0))                                                                \
        typedef typename nested::type type;                                                                                 \
    };                                                                                                                      \
    template<class This,class B, class A0, class A1>                                                                        \
    struct result<This(B& block,A0& a0,A1& a1)>                                                                             \
    {                                                                                                                       \
        static A0& a0;                                                                                                      \
        static A1& a1;                                                                                                      \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,userfunc(a0,a1))                                                             \
        typedef typename nested::type type;                                                                                 \
    };                                                                                                                      \
    template<typename B,typename A0>                                                                                        \
    typename boost::result_of<funcname ## impl(B&,A0&)>::type                                                               \
    operator()(B& block, A0 &)const                                                                                         \
    {                                                                                                                       \
        return userfunc();                                                                                                  \
    }                                                                                                                       \
    template<typename B,typename A0,typename A1>                                                                            \
    typename boost::result_of<funcname ## impl(B&,A0&,A1&)>::type                                                           \
    operator()(B& block, A0 &, A1 &)const                                                                                   \
    {                                                                                                                       \
        return userfunc();                                                                                                  \
    }                                                                                                                       \
};                                                                                                                          \
template < class Param1 > struct funcname ## impl< Param1, void,void,void>                                                  \
    : proto::extends< proto::terminal<boost::msm::lpp::tag::actor>::type, funcname ## impl<Param1, void,void,void> >       \
{                                                                                                                           \
    template<class Sig> struct result;                                                                                      \
    template<class This,class B, class A0>                                                                                  \
    struct result<This(B& block,A0& a0)>                                                                                    \
    {                                                                                                                       \
        static B& block;                                                                                                    \
        static A0& a0;                                                                                                      \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,userfunc(Param1()(block,a0)))                                                \
        typedef typename nested::type type;                                                                                 \
    };                                                                                                                      \
    template<class This,class B, class A0, class A1>                                                                        \
    struct result<This(B& block,A0& a0,A1& a1)>                                                                             \
    {                                                                                                                       \
        static B& block;                                                                                                    \
        static A0& a0;                                                                                                      \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,userfunc(Param1()(block,a0)))                                                \
        typedef typename nested::type type;                                                                                 \
    };                                                                                                                      \
    template<typename B,typename A0>                                                                                        \
    typename boost::result_of<funcname ## impl(B&,A0&)>::type                                                               \
    operator()(B& block, A0 & a0)const                                                                                      \
    {                                                                                                                       \
        return userfunc(Param1()(block,a0));                                                                                \
    }                                                                                                                       \
    template<typename B,typename A0,typename A1>                                                                            \
    typename boost::result_of<funcname ## impl(B&,A0&,A1&)>::type                                                           \
    operator()(B& block, A0 & a0, A1 & a1)const                                                                             \
    {                                                                                                                       \
        return userfunc(Param1()(block,a0,a1));                                                                             \
    }                                                                                                                       \
};                                                                                                                          \
struct funcname ## helper : proto::extends< proto::terminal< boost::msm::lpp::tag::actor >::type,funcname ## helper>       \
{                                                                                                                           \
    funcname ## helper(){}                                                                                                  \
    template <class Arg1,class Arg2,class Arg3,class Arg4>                                                                  \
    struct In {typedef funcname ## impl<Arg1,Arg2,Arg3,Arg4> type;};                                                        \
};                                                                                                                          \
funcname ## helper const funcname;

} } }// boost::msm::lpp
#endif //BOOST_MSM_LPP_OPERATOR_H

