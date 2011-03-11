// Copyright 2011 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MSM_LPP_OPERATOR_H
#define BOOST_MSM_LPP_OPERATOR_H

#include <boost/preprocessor/tuple/elem.hpp> 
#include <boost/preprocessor/punctuation/comma_if.hpp> 
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#include <boost/msm/lpp/basic_grammar.hpp>

namespace boost { namespace msm { namespace lpp
{

#define BOOST_MSM_LPP_ARGS_HELPER(z, n, unused) ARG ## n & t ## n
#define BOOST_MSM_LPP_ARGS_HELPER2(z, n, unused) static ARG ## n & t ## n;
#define BOOST_MSM_LPP_ARGS_HELPER3(z, n, unused) t ## n
#define BOOST_MSM_LPP_ARGS_HELPER4(z, n, unused) ARG ## n &

#define BOOST_MSM_LPP_BINARY_OPERATOR_EXECUTE(z, n, data)                                                                   \
    template <class This,class B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                   \
    struct result<This(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER, ~ ) )>                     \
    {                                                                                                                       \
        static B& block;                                                                                                    \
        BOOST_PP_REPEAT(n, BOOST_MSM_LPP_ARGS_HELPER2, ~)                                                                   \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,T1()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ )) BOOST_PP_TUPLE_ELEM(2, 1, data) T2()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ ))) \
        typedef typename nested::type type;                                                                                 \
    };                                                                                                                      \
    template<typename B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                            \
    typename boost::result_of<BOOST_PP_TUPLE_ELEM(2, 0, data)(B& BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER4, ~ ) )>::type       \
    operator()(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER, ~ ) )const                         \
    {                                                                                                                       \
        return T1()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ ) ) BOOST_PP_TUPLE_ELEM(2, 1, data) T2()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ ));  \
    }                                                                                                   


#define BOOST_MSM_LPP_BINARY_OPERATOR(functor,op)                                                                           \
template <class T1,class T2>                                                                                                \
struct functor ## _ : proto::extends<typename proto::terminal<action_tag>::type, functor ## _<T1,T2> >                      \
{                                                                                                                           \
    template<class Sig> struct result;                                                                                      \
    BOOST_PP_REPEAT(5, BOOST_MSM_LPP_BINARY_OPERATOR_EXECUTE, (functor ## _ ,op) )                                          \
};                                                                                                                          \
template<> struct BuildLambdaCases::case_<proto::tag::functor>                                                          \
	: proto::when<                                                                                                          \
            proto::functor<BuildLambda,BuildLambda >,                                                                   \
            functor ## _<BuildLambda(proto::_left),BuildLambda(proto::_right)>()                                            \
		    >{};

// binary operators
BOOST_MSM_LPP_BINARY_OPERATOR(shift_left,<<)
BOOST_MSM_LPP_BINARY_OPERATOR(shift_right,>>)
BOOST_MSM_LPP_BINARY_OPERATOR(multiplies,*)
BOOST_MSM_LPP_BINARY_OPERATOR(divides,/)
BOOST_MSM_LPP_BINARY_OPERATOR(modulus,%)
BOOST_MSM_LPP_BINARY_OPERATOR(plus,+)
BOOST_MSM_LPP_BINARY_OPERATOR(minus,-)
BOOST_MSM_LPP_BINARY_OPERATOR(less,<)
BOOST_MSM_LPP_BINARY_OPERATOR(greater,>)
BOOST_MSM_LPP_BINARY_OPERATOR(less_equal,<=)
BOOST_MSM_LPP_BINARY_OPERATOR(greater_equal,>=)
BOOST_MSM_LPP_BINARY_OPERATOR(equal_to,==)
BOOST_MSM_LPP_BINARY_OPERATOR(not_equal_to,!=)
BOOST_MSM_LPP_BINARY_OPERATOR(logical_or,||)
BOOST_MSM_LPP_BINARY_OPERATOR(logical_and,&&)
BOOST_MSM_LPP_BINARY_OPERATOR(bitwise_and,&)
BOOST_MSM_LPP_BINARY_OPERATOR(bitwise_or,|)
BOOST_MSM_LPP_BINARY_OPERATOR(bitwise_xor,^)
// not comma, used as ;
BOOST_MSM_LPP_BINARY_OPERATOR(mem_ptr,->*)
BOOST_MSM_LPP_BINARY_OPERATOR(assign,=)
BOOST_MSM_LPP_BINARY_OPERATOR(shift_left_assign,>>=)
BOOST_MSM_LPP_BINARY_OPERATOR(shift_right_assign,<<=)
BOOST_MSM_LPP_BINARY_OPERATOR(multiplies_assign,*=)
BOOST_MSM_LPP_BINARY_OPERATOR(divides_assign,/=)
BOOST_MSM_LPP_BINARY_OPERATOR(modulus_assign,%=)
BOOST_MSM_LPP_BINARY_OPERATOR(plus_assign,+=)
BOOST_MSM_LPP_BINARY_OPERATOR(minus_assign,-=)
BOOST_MSM_LPP_BINARY_OPERATOR(bitwise_and_assign,&=)
BOOST_MSM_LPP_BINARY_OPERATOR(bitwise_or_assign,|=)
BOOST_MSM_LPP_BINARY_OPERATOR(bitwise_xor_assign,^=)



#undef BOOST_MSM_LPP_BINARY_OPERATOR_EXECUTE
#undef BOOST_MSM_LPP_BINARY_OPERATOR

#define BOOST_MSM_LPP_PRE_UNARY_OPERATOR_EXECUTE(z, n, data)                                                                    \
    template <class This,class B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                   \
    struct result<This(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER, ~ ) )>                     \
    {                                                                                                                       \
        static B& block;                                                                                                    \
        BOOST_PP_REPEAT(n, BOOST_MSM_LPP_ARGS_HELPER2, ~)                                                                   \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,BOOST_PP_TUPLE_ELEM(2, 1, data)T1()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ )) ) \
        typedef typename nested::type& type;                                                                                \
    };                                                                                                                      \
    template<typename B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                            \
    typename boost::result_of<BOOST_PP_TUPLE_ELEM(2, 0, data)(B& BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER4, ~ ) )>::type       \
    operator()(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER, ~ ) )const                         \
    {                                                                                                                       \
        return BOOST_PP_TUPLE_ELEM(2, 1, data)T1()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ ) );  \
    }        

#define BOOST_MSM_LPP_PRE_UNARY_OPERATOR(functor,op)                                                                        \
template <class T1>                                                                                                         \
struct functor ## _ : proto::extends<typename proto::terminal<action_tag>::type, functor ## _ <T1> >                        \
{                                                                                                                           \
    template<class Sig> struct result;                                                                                      \
    BOOST_PP_REPEAT(5, BOOST_MSM_LPP_PRE_UNARY_OPERATOR_EXECUTE, (functor ## _ ,op) )                                       \
};                                                                                                                          \
template<> struct BuildLambdaCases::case_<proto::tag::functor>                                                          \
    : proto::when<                                                                                                          \
            proto::functor <BuildLambda >,                                                                              \
            functor ## _< BuildLambda(proto::_child)>()                                                                     \
            >{};

// unary prefix operators
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(unary_plus,+)
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(negate,-)
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(dereference,*)
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(complement,~)
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(address_of,&)
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(logical_not,!)
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(pre_inc,++)
BOOST_MSM_LPP_PRE_UNARY_OPERATOR(pre_dec,--)



#undef BOOST_MSM_LPP_PRE_UNARY_OPERATOR_EXECUTE
#undef BOOST_MSM_LPP_PRE_UNARY_OPERATOR


#define BOOST_MSM_LPP_POST_UNARY_OPERATOR_EXECUTE(z, n, data)                                                                    \
    template <class This,class B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                   \
    struct result<This(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER, ~ ) )>                     \
    {                                                                                                                       \
        static B& block;                                                                                                    \
        BOOST_PP_REPEAT(n, BOOST_MSM_LPP_ARGS_HELPER2, ~)                                                                   \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested,T1()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ ))BOOST_PP_TUPLE_ELEM(2, 1, data) ) \
        typedef typename nested::type type;                                                                                 \
    };                                                                                                                      \
    template<typename B BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class ARG)>                                            \
    typename boost::result_of<BOOST_PP_TUPLE_ELEM(2, 0, data)(B& BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER4, ~ ) )>::type       \
    operator()(B& block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER, ~ ) )const                         \
    {                                                                                                                       \
        return T1()(block BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM(n, BOOST_MSM_LPP_ARGS_HELPER3, ~ ) )BOOST_PP_TUPLE_ELEM(2, 1, data);  \
    }        

#define BOOST_MSM_LPP_POST_UNARY_OPERATOR(functor,op)                                                                       \
template <class T1>                                                                                                         \
struct functor ## _ : proto::extends<typename proto::terminal<action_tag>::type, functor ## _<T1> >                         \
{                                                                                                                           \
    template<class Sig> struct result;                                                                                      \
    BOOST_PP_REPEAT(5, BOOST_MSM_LPP_POST_UNARY_OPERATOR_EXECUTE, (functor ## _ ,op) )                                      \
};                                                                                                                          \
template<> struct BuildLambdaCases::case_<proto::tag::functor>                                                          \
    : proto::when<                                                                                                          \
            proto::functor <BuildLambda >,                                                                              \
            functor ## _< BuildLambda(proto::_child)>()                                                                     \
            >{};

// unary postfix operators
BOOST_MSM_LPP_POST_UNARY_OPERATOR(post_inc,++)
BOOST_MSM_LPP_POST_UNARY_OPERATOR(post_dec,--)

#undef BOOST_MSM_LPP_POST_UNARY_OPERATOR_EXECUTE
#undef BOOST_MSM_LPP_POST_UNARY_OPERATOR

#undef BOOST_MSM_LPP_ARGS_HELPER
#undef BOOST_MSM_LPP_ARGS_HELPER2
#undef BOOST_MSM_LPP_ARGS_HELPER3
#undef BOOST_MSM_LPP_ARGS_HELPER4

} } }// boost::msm::lpp
#endif //BOOST_MSM_LPP_OPERATOR_H

