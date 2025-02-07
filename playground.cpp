#include <boost/mp11.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/mpl.hpp>
#include <boost/mpl/map.hpp>
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

using Set = mp11::mp_list<char, int, double, float>;
using Indices = mp11::mp_iota_c<mp11::mp_size<Set>::value>;
using Map = mp11::mp_transform_q<mp11::mp_bind<mp11::mp_list, mp11::_1, mp11::_2>, Set, Indices>;

using Map = boost::mp11::mp_list<
    boost::mp11::mp_list<char, std::integral_constant<unsigned long, 0>>,
    boost::mp11::mp_list<int, std::integral_constant<unsigned long, 1>>,
    boost::mp11::mp_list<double, std::integral_constant<unsigned long, 2>>,
    boost::mp11::mp_list<float, std::integral_constant<unsigned long, 3>>>;

using Set = mp11::mp_list<char, int, double, float>;
using HighestId = mp11::mp_size_t<mp11::mp_size<Set>::value-1>;


template<typename Seq>
struct integer_sequence_to_mp_list;

template<typename T, T... Ints>
struct integer_sequence_to_mp_list<std::integer_sequence<T, Ints...>> {
    using type = mp11::mp_list_c<T, Ints...>; // Converts directly to mp_list_c
};

using SequenceList = mp11::mp_iota_c<mp11::mp_size<Set>::value>;

typedef mp11::mp_transform_q<
    mp11::mp_bind<mp11::mp_list, mp11::_1, mp11::_2>,
    Set,
    SequenceList
    > source_state_map;

template<typename L, typename V>
using F = mpl::insert<V, L>;

using MplMap = mpl::map<mpl::pair<int, unsigned>>;
using MplMapInsert = typename mpl::insert<MplMap, mpl::pair<size_t, int>::type>::type;

static_assert(mpl::has_key<MplMapInsert, size_t>::value);

typedef mp11::mp_fold<
    Map,
    mpl::map<>,
    F
    > test;

// Verify result (compile-time)
static_assert(mp11::mp_second<mp11::mp_map_find<Map, int>>::value == 1,
              "int should be mapped to 1");

int main() {
  return 0;
}