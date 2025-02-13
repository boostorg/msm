#include <boost/mp11/tuple.hpp>
#include <iostream>

#include <boost/mp11.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/mpl.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/greater.hpp>
#include <cstdint>
#include <boost/mpl/size.hpp>
#include <type_traits>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/for_each.hpp>

/*

https://github.com/boostorg/mp11/issues/65
https://www.boost.org/doc/libs/1_87_0/libs/mp11/doc/html/mp11.html#mpl
https://www.boost.org/doc/libs/1_87_0/libs/mpl/doc/refmanual.pdf

*/

namespace mpl = boost::mpl;
namespace mp11 = boost::mp11;

using my_list = mp11::mp_list<int, size_t>;

int main() {
    size_t cnt = 0;
    mp11::mp_rename<my_list, std::tuple> tuple;
    std::get<int>(tuple) = 2;

    mp11::tuple_for_each(tuple, [](auto elem) {
        std::cout << elem << std::endl;
    });

    return 0;
}