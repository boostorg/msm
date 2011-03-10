// Copyright 2011 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MSM_LPP_BASIC_GRAMMAR_H
#define BOOST_MSM_LPP_BASIC_GRAMMAR_H

#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>
#include <boost/fusion/container/list/cons.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/utility/result_of.hpp>


namespace boost { namespace msm { namespace lpp
{
struct action_tag {};

template <class T,class Arg1=void,class Arg2=void,class Arg3=void,class Arg4=void>
struct get_fct 
{
    typedef typename T::template In<Arg1,Arg2,Arg3,Arg4>::type type;
};

// wrapper for mpl::for_each as showed in the C++ Template Metaprogramming ch. 9
template <class T>
struct wrap{};

template <class Sequence>
struct LambdaSequence_
{
    typedef Sequence sequence;
    typedef typename ::boost::mpl::deref< typename ::boost::mpl::prior< typename ::boost::mpl::end<Sequence>::type >::type>::type last;

    template<class Sig> struct result;
    template<class This,class B, class A0> 
    struct result<This(B& block,A0& a0)>
    {
        static B& block;
        static A0& a0;
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,last()(block,a0) )
        typedef typename nested::type type;
    };
    template<class This,class B, class A0, class A1> 
    struct result<This(B& block,A0& a0,A1& a1)>
    {
        static B& block;
        static A0& a0;
        static A1& a1;
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,last()(block,a0,a1) )
        typedef typename nested::type type;
    };

    template <typename B,class A0,class A1=void>
    struct Call
    {
        Call(B& block,A0& a0,A1& a1): block_(block),a0_(a0),a1_(a1){}
        template <class FCT>
        void operator()(wrap<FCT> const& )
        {
            FCT()(block_,a0_,a1_);
        }
    private:
        B&   block_;
        A0&  a0_;
        A1&  a1_;
    };
    template <typename B,class A0>
    struct Call<B,A0,void>
    {
        Call(B& block,A0& a0): block_(block),a0_(a0){}
        template <class FCT>
        void operator()(wrap<FCT> const& )
        {
            FCT()(block_,a0_);
        }
        private:
            B&   block_;
            A0&  a0_;
    };

    template<typename B,typename A0>
    typename boost::result_of<LambdaSequence_(B&,A0&)>::type
    operator ()(B& block, A0 & a0) const
    {
        mpl::for_each< typename ::boost::mpl::pop_back<Sequence>::type,
                       wrap< ::boost::mpl::placeholders::_1> >
            (Call<B,A0>(block,a0));
        return last()(block,a0);
    }
    template<typename B,typename A0,typename A1>
    typename boost::result_of<LambdaSequence_(B&,A0&,A1&)>::type
    operator ()(B& block, A0 & a0, A1 & a1) const
    {
        mpl::for_each< typename ::boost::mpl::pop_back<Sequence>::type,
                       wrap< ::boost::mpl::placeholders::_1> >
            (Call<B,A0,A1>(block,a0,a1));
        return last()(block,a0,a1);
    }
};

struct if_then_tag 
{
};
template <class T>
struct get_condition 
{
    typedef typename T::condition type;
};
template <class T>
struct get_ifclause 
{
    typedef typename T::ifclause type;
};
struct ignored {};
template <class Condition,class IfClause=void>
struct if_then_
{
    typedef Condition condition;
    typedef IfClause ifclause;

    template<class Sig> struct result;
    template<class This,class B, class A0> 
    struct result<This(B& block,A0& a0)>
    {
        typedef ignored type;
    };
    template<class This,class B, class A0, class A1> 
    struct result<This(B& block,A0& a0,A1& a1)>
    {
        typedef ignored type;
    };
    template<typename B,typename A0>
    typename boost::result_of<if_then_(B&,A0&)>::type
    operator()(B& block, A0 & a0)const
    {
        if (Condition()(block,a0))
        {
            IfClause()(block,a0);
        }
        return ignored();
    }
    template<typename B,typename A0,typename A1>
    typename boost::result_of<if_then_(B&,A0&,A1&)>::type
    operator()(B& block, A0 & a0, A0 & a1)const
    {
        if (Condition()(block,a0,a1))
        {
            IfClause()(block,a0,a1);
        }
        return ignored();
    }
};

struct if_then_helper : proto::extends< proto::terminal<if_then_tag>::type, if_then_helper >
{
    template <class Arg1,class Arg2,class Arg3,class Arg4>
    struct In {typedef if_then_<Arg1,Arg2> type;};
};
if_then_helper const if_;


template <class Condition,class IfClause,class ElseClause>
struct if_then_else
{
    template<class Sig> struct result;
    template<class This,class B, class A0> 
    struct result<This(B& block,A0& a0)>
    {
        static B& block;
        static A0& a0;
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,IfClause()(block,a0))
        typedef typename nested::type type;
    };
    template<class This,class B, class A0, class A1> 
    struct result<This(B& block,A0& a0,A1& a1)>
    {
        static B& block;
        static A0& a0;
        static A1& a1;
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,IfClause()(block,a0,a1))
        typedef typename nested::type type;
    };
    template<typename B,typename A0>
    typename boost::result_of<if_then_else(B&,A0&)>::type
    operator()(B& block, A0 & a0)const
    {
        if (Condition()(block,a0))
        {
            return IfClause()(block,a0);
        }
        else
        {
            return ElseClause()(block,a0);
        }
    }
    template<typename B,typename A0,typename A1>
    typename boost::result_of<if_then_else(B&,A0&,A1&)>::type
    operator()(B& block, A0 & a0, A0 & a1)const
    {
        if (Condition()(block,a0,a1))
        {
            return IfClause()(block,a0,a1);
        }
        else
        {
            return ElseClause()(block,a0,a1);
        }
    }
};


struct BuildLambda;

struct BuildLambdaSequence
    : proto::or_<
            proto::when< proto::comma< BuildLambdaSequence, BuildLambdaSequence >,
                         proto::fold_tree<
                              proto::_
                            , ::boost::mpl::vector0<>()
                            , ::boost::mpl::push_back<proto::_state, BuildLambda(proto::_) >()
                         >

            >,
            proto::when< BuildLambda,
                        ::boost::mpl::vector1<BuildLambda>()
            >
    >
{};


struct BuildLambdaCases
{
    // The primary template matches nothing:
    template<typename Tag>
    struct case_
        : proto::not_<proto::_>
    {};
};

template<>
struct BuildLambdaCases::case_<proto::tag::terminal>
    :   proto::or_<
			proto::when<
				proto::terminal< proto::_ >,
				proto::_child
			>
        >
{};

template<>
struct BuildLambdaCases::case_<proto::tag::function>
    : proto::or_<
            proto::when<
                    proto::function<proto::terminal<proto::_> >,
                    get_fct< proto::_child_c<0> >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildLambda >,
                    get_fct<proto::_child_c<0>,BuildLambda(proto::_child_c<1>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildLambda,BuildLambda >,
                    get_fct<proto::_child_c<0>,BuildLambda(proto::_child_c<1>),BuildLambda(proto::_child_c<2>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildLambda,BuildLambda,BuildLambda >,
                    get_fct<proto::_child_c<0>,BuildLambda(proto::_child_c<1>),BuildLambda(proto::_child_c<2>),
                                               BuildLambda(proto::_child_c<3>) >()
                    >,
            proto::when<
                    proto::function<proto::terminal<proto::_>,BuildLambda,BuildLambda,BuildLambda,BuildLambda >,
                    get_fct<proto::_child_c<0>,BuildLambda(proto::_child_c<1>),BuildLambda(proto::_child_c<2>),
                                               BuildLambda(proto::_child_c<3>),BuildLambda(proto::_child_c<4>) >()
                    >
    >
{};

struct IfThenCondition:
    proto::when<
                proto::function<proto::terminal<if_then_tag>,BuildLambdaSequence >,
                if_then_< LambdaSequence_<BuildLambdaSequence(proto::_child_c<1>)>() >()

            >
{};
struct IfThenGrammar:
    proto::when<
                proto::subscript< IfThenCondition , BuildLambdaSequence >,
                if_then_< get_condition<IfThenCondition(proto::_child_c<0>)>(),
                          LambdaSequence_<BuildLambdaSequence(proto::_child_c<1>)>() >()
            >
{};
struct IfThenElseGrammar:
    proto::when<
                proto::subscript< IfThenGrammar , BuildLambdaSequence >,
                if_then_else< get_condition<IfThenGrammar(proto::_child_c<0>)>,
                              get_ifclause<IfThenGrammar(proto::_child_c<0>)>,
                              LambdaSequence_<BuildLambdaSequence(proto::_child_c<1>)>() >()
            >
{};
template<>
struct BuildLambdaCases::case_<proto::tag::subscript>
    : proto::or_<
            IfThenGrammar,
            IfThenElseGrammar
    >
{};

struct BuildLambda
    : proto::switch_<BuildLambdaCases>
{};

struct params_tag{};
// Eric Niebler's FoldToList
struct FoldToList
  : proto::or_<
        // Don't add the params_ terminal to the list
        proto::when<
            proto::terminal< params_tag >
          , proto::_state
        >
        // Put all other terminals at the head of the
        // list that we're building in the "state" parameter
      , proto::when<
            proto::terminal< proto::_>
            , boost::fusion::cons<proto::_value, proto::_state>(
                proto::_value, proto::_state
            )
        >
        // For left-shift operations, first fold the right
        // child to a list using the current state. Use
        // the result as the state parameter when folding
        // the left child to a list.
      , proto::when<
            proto::shift_left<FoldToList, FoldToList>
          , FoldToList(
                proto::_left
              , proto::call<FoldToList(proto::_right, proto::_state)>
            )
        >
    >
{};


} } }// boost::msm::lpp
#endif //BOOST_MSM_LPP_BASIC_GRAMMAR_H

