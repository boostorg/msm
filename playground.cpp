#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/vector_c.hpp>

/*

https://github.com/boostorg/mp11/issues/65
https://www.boost.org/doc/libs/1_87_0/libs/mp11/doc/html/mp11.html#mpl
https://www.boost.org/doc/libs/1_87_0/libs/mpl/doc/refmanual.pdf

*/

namespace mpl = boost::mpl;
namespace mp11 = boost::mp11;


int main()
{
    typedef mpl::vector_c<int,1,2,3,4> numbers;
    typedef mpl::find<numbers,mpl::integral_c<int,4>>::type pos;
    

    return 0;
}