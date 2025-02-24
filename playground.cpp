#include <boost/mp11.hpp>
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


namespace mpl = boost::mpl;
namespace mp11 = boost::mp11;

using test = mp11::mp_append<mp11::mp_list<int>, mp11::mp_list<>>;


// void test1()
// {
//     static auto array_1 = create_array_cpp11<cell_constants_1>();
//     static auto array_2 = create_array_cpp11<cell_constants_2>();
//     static auto array_5 = create_array_cpp11<cell_constants_5>();
// }



int main() {
    // auto array = create_array_cpp17<table_initializer>();
    // test1();
    // test2();
    // test3();
    return 0;
}