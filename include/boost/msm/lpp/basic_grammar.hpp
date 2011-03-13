// Copyright 2011 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MSM_LPP_BASIC_GRAMMAR_H
#define BOOST_MSM_LPP_BASIC_GRAMMAR_H

#include <boost/fusion/container/list/cons.hpp>

#include <boost/msm/lpp/common_types.hpp>


namespace boost { namespace msm { namespace lpp
{
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
    proto::function<proto::terminal<tag::if_then>,BuildLambdaSequence >,
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

// Eric Niebler's FoldToList
struct FoldToList
  : proto::or_<
        // Don't add the params_ terminal to the list
        proto::when<
        proto::terminal< tag::capture >
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

struct BuildParams
    : proto::or_<
        proto::when<
        proto::subscript< proto::terminal<tag::lambda> , FoldToList >,
            FoldToList(proto::_child_c<1>, boost::fusion::nil())
        >
    >
{};

struct Lambda:
    proto::when<
        BuildLambdaSequence, LambdaSequence_<BuildLambdaSequence(proto::_child_c<1>) >()
    >
{};
#define BOOST_MSM_LPP_LAMBDA_EXPR(expr) BOOST_TYPEOF( Lambda()(expr) ) 



struct BuildLambdaWithParams
    : proto::or_<
        proto::when<
        proto::subscript< proto::terminal<tag::lambda> ,BuildLambdaSequence>,
            Block<LambdaSequence_<BuildLambdaSequence(proto::_child_c<1>) > >()
        >,
        proto::when<
            proto::subscript< BuildParams , BuildLambdaSequence >,
            Block<
                LambdaSequence_<BuildLambdaSequence(proto::_child_c<1>)>, 
                boost::fusion::result_of::as_vector<BuildParams(proto::_child_c<0>)>  >
                    ( (BuildParams(proto::_value) ) )
        >
    >
{};

template<typename Expr>
struct lambda_expr;

struct lambda_dom
  : proto::domain<proto::pod_generator<lambda_expr>, BuildLambdaWithParams>
{};

template<typename Expr>
struct lambda_expr
{
    BOOST_PROTO_BASIC_EXTENDS(Expr, lambda_expr<Expr>, lambda_dom)
    BOOST_PROTO_EXTENDS_SUBSCRIPT() 
    template<class Sig> struct result;
    template<class This,class B, class A0> 
    struct result<This(B& block,A0& a0)>
    {
        static A0& a0;
        typedef A0& type;
    };
    template<typename A0>
    typename ::boost::result_of<
        typename ::boost::result_of<BuildLambdaWithParams(Expr,A0&)>::type (A0&)
    >::type
    operator()(A0 &a0)const
    {
        BOOST_MPL_ASSERT((proto::matches<Expr, BuildLambdaWithParams>));
        return BuildLambdaWithParams()(*this)(a0);
    }
    template<typename A0>
    typename ::boost::result_of<
        typename ::boost::result_of<BuildLambdaWithParams(Expr,A0&)>::type (A0&)
    >::type
    operator()(A0 const &a0)const
    {
        BOOST_MPL_ASSERT((proto::matches<Expr, BuildLambdaWithParams>));
        return BuildLambdaWithParams()(*this)(a0);
    }

    template<typename A0,typename A1>
    typename ::boost::result_of<
        typename ::boost::result_of<BuildLambdaWithParams(Expr,A0&,A1&)>::type (A0&,A1&)
    >::type
    operator()(A0 &a0,A1 &a1)const
    {
        BOOST_MPL_ASSERT((proto::matches<Expr, BuildLambdaWithParams>));
        return BuildLambdaWithParams()(*this)(a0,a1);
    }
    template<typename A0,typename A1>
    typename ::boost::result_of<
        typename ::boost::result_of<BuildLambdaWithParams(Expr,A0&,A1&)>::type (A0&,A1&)
    >::type
    operator()(A0 const &a0,A1 const &a1)const
    {
        BOOST_MPL_ASSERT((proto::matches<Expr, BuildLambdaWithParams>));
        return BuildLambdaWithParams()(*this)(a0,a1);
    }
};

lambda_expr<proto::terminal<tag::lambda>::type> const lambda = {{{}}};

} } }// boost::msm::lpp
#endif //BOOST_MSM_LPP_BASIC_GRAMMAR_H

