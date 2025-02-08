#include <boost/mp11.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/mpl.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/at.hpp>
#include <cstdint>
#include <boost/mpl/size.hpp>
#include <type_traits>

/*

https://github.com/boostorg/mp11/issues/65
https://www.boost.org/doc/libs/1_87_0/libs/mp11/doc/html/mp11.html#mpl
https://www.boost.org/doc/libs/1_87_0/libs/mpl/doc/refmanual.pdf

*/

namespace mpl = boost::mpl;
namespace mp11 = boost::mp11;

using Set = mp11::mp_list<char, int, double, float>;
using Indices = mp11::mp_iota_c<mp11::mp_size<Set>::value>;
using Map = mp11::mp_transform_q<mp11::mp_bind<mp11::mp_list, mp11::_1, mp11::_2>, Set, Indices>;

using Map = boost::mp11::mp_list<
    boost::mp11::mp_list<char, std::integral_constant<unsigned long, 0>>,
    boost::mp11::mp_list<int, std::integral_constant<unsigned long, 1>>,
    boost::mp11::mp_list<double, std::integral_constant<unsigned long, 2>>,
    boost::mp11::mp_list<float, std::integral_constant<unsigned long, 3>>>;

// Verify result (compile-time)
static_assert(mp11::mp_second<mp11::mp_map_find<Map, int>>::value == 1,
              "int should be mapped to 1");



template<typename V, typename T>
using F = mp11::mp_if_c<
    mpl::has_key<V, T>::value,
    V,
    typename mpl::insert<V, mpl::pair<T, typename mpl::size<V>::type>>::type>;
// using F = typename mpl::insert<V, mpl::pair<T, typename mpl::size<V>::type>>::type;
typedef mp11::mp_fold<
    mp11::mp_list<char, size_t>,
    mpl::map<>,
    F
    > MplMap;

using MplMap2 = mpl::map<mpl::pair<char, bool>>;

using EmptyMplMap = mpl::map<>;
using MplMap3 = typename mpl::insert<EmptyMplMap, mpl::pair<char, bool>>::type;

static_assert(mpl::has_key<MplMap, char>::value);
static_assert(std::is_same_v<typename mpl::at<MplMap,char>::type, mpl_::long_<0>>);
static_assert(mpl::has_key<MplMap, size_t>::value);
static_assert(std::is_same_v<typename mpl::at<MplMap,size_t>::type, mpl_::long_<1>>);



template<typename T, typename U>
using item_pusher = mp11::mp_push_back<U, T>;
template<typename T, typename V>
using map_updater = mp11::mp_map_update<
    V,
    mp11::mp_list<
        T,
        mp11::mp_list<>
        >,
    item_pusher
    >;

using Test = mp11::mp_reverse_fold<
    mp11::mp_list<size_t, uint16_t, uint8_t, uint8_t>,
    mp11::mp_list<>,
    map_updater
    >;





int main() {
  return 0;
}