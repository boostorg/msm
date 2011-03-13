// Copyright 2011 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <boost/msm/lpp/lpp.hpp>
#include <boost/assert.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;

using namespace boost::msm::lpp;

int foo(int p){return p+1;}
int foo(){return 0;}

BOOST_MSM_LPP_FUNCTION(foo_,foo)



int main()
{

    // 1. STL type definition
    std::set<int, BOOST_MSM_LPP_LAMBDA_EXPR(_1 < _2) > s2;
    // same as
    std::set<int, BOOST_TYPEOF( Lambda()(_1 < _2) ) > s;

    // 2. simple STL algorithm
    std::vector<int> vec(12);

    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[ ++_1] );
    BOOST_ASSERT(vec[10] == 1);

    // 3. sequence of statements ++_1; ++_1; return ++_1
    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[ ++_1, ++_1 , ++_1 ] );
    BOOST_ASSERT(vec[10] == 4);

    // 4. More complicated, and return value is the last statement called
    std::transform(vec.begin(),vec.end(),vec.begin(),
        lambda[ if_(_1)[ ++_1, ++_1 ] /*else */  [ ++_1 ]  ] );
    BOOST_ASSERT(vec[10] == 6);

    // 5. a taste of 0x: capturing
    int q=4;
    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[_capture << q ][ _c1 + _1 ] );
    BOOST_ASSERT(vec[10] == 10); // was 6, now 6+4

    // 5.1 capturing by value (_c1 not modified)
    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[_capture << q ][ ++_c1 ] );
    BOOST_ASSERT(vec[10] == 5); //always 4+1
   
    // 5.2 capturing by ref (_c1 modified at each call)
    int p=0;
    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[_capture << boost::ref(p) ][ _c1++ ] );
    BOOST_ASSERT(vec[0] == 0);
    BOOST_ASSERT(vec[1] == 1);
    BOOST_ASSERT(vec[2] == 2);

    // 5.3 capturing by cref
    //this will correctly will not compile (const)
    //std::transform(vec.begin(),vec.end(),vec.begin(),lambda[_params << boost::cref(p) ][ _c1++ ] );
    // this will: _c1 not modified
    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[_capture << boost::cref(q) ][ _1 + _c1 ] );
    BOOST_ASSERT(vec[1] == 5); // 1 + 4

    // 6. using user functions
    // int foo(int p){return p+1;}
    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[ _1 + foo_(_1)] );
    BOOST_ASSERT(vec[1] == 11); // 5 from before + (5+1)

    std::transform(vec.begin(),vec.end(),vec.begin(),vec.begin(),lambda[_capture << 4 ][ foo_(_1 + _c1) ] );
    BOOST_ASSERT(vec[1] == 16); // foo(11 + 4)

    // 7.  a bit of everything ;-)

    // 2nd form of transform
    std::transform(vec.begin(),vec.end(),vec.begin(),vec.begin(),lambda[_capture << 2 ][ (_1 + _2) * _c1] );
    BOOST_ASSERT(vec[1] == 64); // (16+16)*2

    // if clause executing 2 statements, returning the second one (not C++ ;-) )
    vec[1] = 1;
    std::for_each(vec.begin(),vec.end(),lambda[ if_((++_1,_1>=_1)) /* then */ [++_1] ] );
    BOOST_ASSERT(vec[1] == 3); // 1 + if(++_1, true) then ++_1 => 1 + 1 + 1 

    // completely useless but so fun :)
    vec[1] = 1;
    std::transform(vec.begin(),vec.end(),vec.begin(),lambda[_capture << 10 ]
        [ 
            if_(_1) /* true */
            [
                if_(_1 + _c1) /* true */
                [
                    ++_1,++_1 /* _1+2 */
                ]
                /* else */
                [
                    --(--_1)
                ] 
            ]
            /* else */
            [
                --_1,--_1
            ] 
        ] );

    BOOST_ASSERT(vec[1] == 3); // 1 + 2

    std::cout << "we survived all this :)" << std::endl;
    // note: On a Q6600, compiles in:
    // 9s with VC9
    // 8.6 with g++ 4.4 MinGW
    return 0;
}

