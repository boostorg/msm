// Copyright 2011 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MSM_LPP_COMMON_TYPES_H
#define BOOST_MSM_LPP_COMMON_TYPES_H


#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/as_vector.hpp>

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/msm/lpp/basic_grammar.hpp>


namespace boost { namespace msm { namespace lpp
{

struct lambda_tag{};

template <class T>
struct get_nested_type
{
    typedef typename ::boost::add_reference<typename T::type>::type type;
};

template<typename B,int position>
struct lambda_parameter_result
{
    typedef typename ::boost::remove_reference<
        typename boost::fusion::result_of::at_c<typename B::params,position>::type
    >::type contained_type;

    typedef typename boost::mpl::eval_if< 
        typename ::boost::is_reference_wrapper< contained_type >::type,
        get_nested_type< contained_type >,
        boost::fusion::result_of::at_c<typename B::params,position>
    >::type type;
};

template<int I>
struct lambda_parameter;

template<>
struct lambda_parameter<0>
{
    template<class Sig> struct result;
    template<class This,class B, class A0> 
    struct result<This(B& block,A0& a0)>
    {
        static B& block;
        static A0& a0;
        typedef typename lambda_parameter_result<B,0>::type type;
    };
    template<class This,class B, class A0, class A1> 
    struct result<This(B& block,A0& a0,A1& a1)>
    {
        static B& block;
        static A0& a0;
        static A1& a1;
        typedef typename lambda_parameter_result<B,0>::type type;
    };

    template<typename B,typename A0>
    typename ::boost::enable_if<
        typename ::boost::is_reference_wrapper< typename lambda_parameter_result<B,0>::contained_type >::type,
        typename boost::result_of<lambda_parameter<0>(B&,A0&)>::type
    >::type
    operator ()(B& block, A0&) const
    {
        return boost::fusion::at_c<0>(block.lambda_params).get();
    }

    template<typename B,typename A0>
    typename ::boost::disable_if<
        typename ::boost::is_reference_wrapper< typename lambda_parameter_result<B,0>::contained_type >::type,
        typename boost::result_of<lambda_parameter<0>(B&,A0&)>::type
    >::type
    operator ()(B& block, A0&) const
    {
        return boost::fusion::at_c<0>(block.lambda_params);
    }
    template<typename B,typename A0,typename A1>
    typename ::boost::enable_if<
        typename ::boost::is_reference_wrapper< typename lambda_parameter_result<B,0>::contained_type >::type,
        typename boost::result_of<lambda_parameter<0>(B&,A0&,A1&)>::type
    >::type
    operator ()(B& block, A0&, A1&) const
    {
        return boost::fusion::at_c<0>(block.lambda_params).get();
    }

    template<typename B,typename A0,typename A1>
    typename ::boost::disable_if<
        typename ::boost::is_reference_wrapper< typename lambda_parameter_result<B,0>::contained_type >::type,
        typename boost::result_of<lambda_parameter<0>(B&,A0&,A1&)>::type
    >::type
    operator ()(B& block, A0&, A1&) const
    {
        return boost::fusion::at_c<0>(block.lambda_params);
    }
};
proto::terminal<lambda_parameter< 0 > >::type const _p1={{}};

template<int I>
struct placeholder;

template<>
struct placeholder<0>
{
    template<class Sig> struct result;
    template<class This,class B, class A0> 
    struct result<This(B& block,A0& a0)>
    {
        static A0& a0;
        typedef A0& type;
    };
    template<class This,class B, class A0, class A1> 
    struct result<This(B& block,A0& a0,A1& a1)>
    {
        static A0& a0;
        typedef A0& type;
    };
    template<typename B,typename A0>
    typename boost::result_of<placeholder<0>(B&,A0&)>::type
    operator ()(B&, A0 &a0) const
    {
        return a0;
    }
    template<typename B,typename A0,typename A1>
    typename boost::result_of<placeholder<0>(B&,A0&,A1&)>::type
    operator ()(B&, A0 &a0, A1 &) const
    {
        return a0;
    }
};
template<>
struct placeholder<1>
{
    template<class Sig> struct result;
    template<class This,class B, class A0, class A1> 
    struct result<This(B& block,A0& a0,A1& a1)>
    {
        static A0& a0;
        static A1& a1;
        typedef A1& type;
    };

    template<typename B,typename A0>
    typename boost::result_of<placeholder<1>(B&,A0&)>::type
    operator ()(B&,A0 &a0) const
    {
        //TODO error
    }

    template<typename B,typename A0, typename A1>
    typename boost::result_of<placeholder<1>(B&,A0&,A1&)>::type
    operator ()(B&,A0 &a0, A1 &a1) const
    {
        return a1;
    }
};


// Define some lambda placeholders
proto::terminal<placeholder< 0 > >::type const _1={{}};
proto::terminal<placeholder< 1 > >::type const _2={{}};
proto::terminal<params_tag>::type const _params={{}};



template <class Seq = void, class Params = ::boost::fusion::vector0<> >
struct Block
{
    typedef Seq sequence;
    typedef Params params;

    Block(){}
    template <typename WithParams>
    Block(WithParams const& p):lambda_params(p)
    {
        lambda_params = boost::fusion::as_vector(p);
    }

    template<class Sig> struct result;
    template<class This, class A0> 
    struct result<This(A0& a0)>
    {
        static Block<Seq,Params>& block;
        static A0& a0;
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,Seq()(block,a0) )
        typedef typename nested::type type;
    };
    template<class This, class A0, class A1> 
    struct result<This(A0& a0,A1& a1)>
    {
        static Block<Seq,Params>& block;
        static A0& a0;
        static A1& a1;
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,Seq()(block,a0,a1) )
        typedef typename nested::type type;
    };
    template<typename A0>
    typename boost::result_of<Block(A0&)>::type
    operator()(A0 &a0)
    {
        return Seq()(*this,a0);
    }
    template<typename A0,typename A1>
    typename boost::result_of<Block(A0&,A1&)>::type
        operator()(A0 &a0,A1 &a1)
    {
        return Seq()(*this,a0,a1);
    }
    //attribute 
    params lambda_params;
};


struct BuildParams
    : proto::or_<
        proto::when<
            proto::subscript< proto::terminal<lambda_tag> , FoldToList >,
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
            proto::subscript< proto::terminal<lambda_tag> ,BuildLambdaSequence>,
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

lambda_expr<proto::terminal<lambda_tag>::type> const lambda = {{{}}};

} } }// boost::msm::lpp
#endif //BOOST_MSM_LPP_COMMON_TYPES_H

