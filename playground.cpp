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

/*

https://github.com/boostorg/mp11/issues/65
https://www.boost.org/doc/libs/1_87_0/libs/mp11/doc/html/mp11.html#mpl
https://www.boost.org/doc/libs/1_87_0/libs/mpl/doc/refmanual.pdf

*/

namespace mpl = boost::mpl;
namespace mp11 = boost::mp11;

template<typename T>
using to_mp_list = typename mpl::copy<T, mpl::back_inserter<mp11::mp_list<>>>::type;

using SpecialMapEntry = mpl::pair<int, mp11::mp_list<int, int>>;
using SpecialMapEntry2 = mpl::pair<char, mp11::mp_list<int, int>>;
using SpecialMapInMpl = mpl::map<SpecialMapEntry, SpecialMapEntry2>;

template<typename T, typename U>
using item_pusher = mp11::mp_push_back<U, T, T, size_t, size_t>;
template<typename T, typename M>
using map_updater = mp11::mp_map_update<
    M,
    // first row on this source state, make a list with 1 element
    mp11::mp_list<
        T,
        mp11::mp_list<int>
        >,
    // list already exists, add the row
    item_pusher
    >;


using NewMap = mp11::mp_reverse_fold<
    mp11::mp_list<char, char>,
    mp11::mp_list<>,
    map_updater
    >;

int main() {
  return 0;
}