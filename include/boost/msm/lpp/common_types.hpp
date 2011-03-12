// Copyright 2011 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MSM_LPP_COMMON_TYPES_H
#define BOOST_MSM_LPP_COMMON_TYPES_H


#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/as_vector.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/utility/result_of.hpp>

#include <boost/preprocessor/tuple/elem.hpp> 
#include <boost/preprocessor/punctuation/comma_if.hpp> 
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#include <boost/msm/lpp/tags.hpp>


namespace boost { namespace msm { namespace lpp
{

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
        boost::mpl::for_each< typename ::boost::mpl::pop_back<Sequence>::type,
                       wrap< ::boost::mpl::placeholders::_1> >
            (Call<B,A0>(block,a0));
        return last()(block,a0);
    }
    template<typename B,typename A0,typename A1>
    typename boost::result_of<LambdaSequence_(B&,A0&,A1&)>::type
    operator ()(B& block, A0 & a0, A1 & a1) const
    {
        boost::mpl::for_each< typename ::boost::mpl::pop_back<Sequence>::type,
                       wrap< ::boost::mpl::placeholders::_1> >
            (Call<B,A0,A1>(block,a0,a1));
        return last()(block,a0,a1);
    }
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

struct if_then_helper : proto::extends< proto::terminal<tag::if_then>::type, if_then_helper >
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

#define BOOST_MSM_LPP_CAPTURE_HELPER(z, n, unused) ARG ## n & t ## n
#define BOOST_MSM_LPP_CAPTURE_HELPER2(z, n, unused) static ARG ## n & t ## n;
#define BOOST_MSM_LPP_CAPTURE_HELPER3(z, n, unused) t ## n
#define BOOST_MSM_LPP_CAPTURE_HELPER4(z, n, unused) ARG ## n &

#define BOOST_MSM_LPP_LAMBDA_CAPTURE_EXECUTE(z, n, data)                                                                    \
    template <class This,class B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                   \
    struct result<This(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_CAPTURE_HELPER, ~ ) )>                  \
    {                                                                                                                       \
        static B& block;                                                                                                    \
        BOOST_PP_REPEAT(n, BOOST_MSM_LPP_CAPTURE_HELPER2, ~)                                                                \
        typedef typename lambda_parameter_result<B,data>::type type;                                                        \
    };                                                                                                                      \
    template<typename B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                            \
    typename ::boost::enable_if<                                                                                            \
        typename ::boost::is_reference_wrapper< typename lambda_parameter_result<B,data>::contained_type >::type,           \
        typename boost::result_of<lambda_parameter<data>(B& BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_CAPTURE_HELPER4, ~ ) )>::type  \
    >::type                                                                                                                 \
    operator()(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_CAPTURE_HELPER, ~ ) )const                      \
    {return boost::fusion::at_c<data>(block.lambda_params).get();}                                                          \
    template<typename B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                            \
    typename ::boost::disable_if<                                                                                           \
        typename ::boost::is_reference_wrapper< typename lambda_parameter_result<B,data>::contained_type >::type,           \
        typename boost::result_of<lambda_parameter<0>(B& BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_CAPTURE_HELPER4, ~ ) )>::type \
    >::type                                                                                                                 \
    operator ()(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_CAPTURE_HELPER, ~ ) ) const                    \
    { return boost::fusion::at_c<0>(block.lambda_params); }


#define BOOST_MSM_LPP_LAMBDA_CAPTURE_DEF(index)                                                                             \
    template<> struct lambda_parameter<index> {                                                                             \
    template<class Sig> struct result;                                                                                      \
    BOOST_PP_REPEAT(5, BOOST_MSM_LPP_LAMBDA_CAPTURE_EXECUTE, index )                                                        \
};

#define BOOST_MSM_LPP_LAMBDA_CAPTURE(index,name)                                                                            \
    BOOST_MSM_LPP_LAMBDA_CAPTURE_DEF(index)                                                                                 \
    proto::terminal<lambda_parameter< index > >::type const name={{}};

// we define a few capture names
BOOST_MSM_LPP_LAMBDA_CAPTURE(0,_c1)
BOOST_MSM_LPP_LAMBDA_CAPTURE(1,_c2)
BOOST_MSM_LPP_LAMBDA_CAPTURE(2,_c3)
BOOST_MSM_LPP_LAMBDA_CAPTURE(3,_c4)
BOOST_MSM_LPP_LAMBDA_CAPTURE(4,_c5)


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
proto::terminal<tag::param>::type const _params={{}};



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


} } }// boost::msm::lpp
#endif //BOOST_MSM_LPP_COMMON_TYPES_H

