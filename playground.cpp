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

struct cell
{
    size_t first;
    size_t second;
};

template<size_t v1, size_t v2>
struct cell_constant
{
    static constexpr cell value = {v1, v2};
};

using cell_constants_1 = mp11::mp_list<
    cell_constant<0, 1>,
    cell_constant<2, 3>
    >;

using cell_constants_2 = mp11::mp_list<
    cell_constant<0, 1>,
    cell_constant<2, 3>
    >;

using cell_constants_5 = mp11::mp_list<
    cell_constant<0, 1>,
    cell_constant<2, 3>,
    cell_constant<4, 5>,
    cell_constant<6, 7>,
    cell_constant<8, 9>
    >;

// // Generic function to create an array from an mp_list
// template<typename Table>
// constexpr auto create_array_cpp17()
// {
//     constexpr auto size = mp11::mp_size<Table>::value; // Get number of elements

//     return []<std::size_t... I>(std::index_sequence<I...>)
//     {
//         return std::array{ mp11::mp_at_c<Table, I>::value... }; // Extract values
//     }(std::make_index_sequence<size>{});
// }

// // Helper to extract values recursively (C++14 version)
// template<typename Table, std::size_t... I>
// constexpr auto create_array_cpp14_impl(std::index_sequence<I...>)
//     -> std::array<std::pair<size_t, size_t>, sizeof...(I)>
// {
//     return { mp11::mp_at_c<Table, I>::value... };
// }

// // Main create_array function
// template<typename Table>
// constexpr auto create_array_cpp14()
// {
//     return create_array_cpp14_impl<Table>(std::make_index_sequence<mp11::mp_size<Table>::value>{});
// }

// Helper to extract values recursively (C++14 version)
template<typename Table, std::size_t... I>
constexpr const std::array<cell, sizeof...(I)> create_array_cpp11_impl(mp11::index_sequence<I...>)
{
    return { mp11::mp_at_c<Table, I>::value... };
}
// Main create_array function
template<typename Table>
constexpr const std::array<cell, mp11::mp_size<Table>::value> create_array_cpp11()
{
    return create_array_cpp11_impl<Table>(mp11::make_index_sequence<mp11::mp_size<Table>::value>{});
}


// Helper to extract values recursively (C++14 version)
template<typename Table, std::size_t... I>
const cell* create_array_cpp11_2_impl(mp11::index_sequence<I...>)
{
    static constexpr cell values[] {mp11::mp_at_c<Table, I>::value...};
    return values;
}
template<typename Table>
constexpr const cell* create_array_cpp11_2()
{
    return create_array_cpp11_2_impl<Table>(mp11::make_index_sequence<mp11::mp_size<Table>::value>{});
}



template<typename Table>
struct table_initializer
{
    using type = std::array<cell, mp11::mp_size<Table>::value>;

    static constexpr type init()
    {
        type array;
        mp11::mp_for_each<Table>(table_initializer<Table>{array});
        return array;
    }

    template<size_t v1, size_t v2>
    constexpr void operator()(const cell_constant<v1, v2>& cell_value)
    {
        array[index] = cell_value.value;
        index++;
    }

    type& array;
    size_t index{0};
};


// void test1()
// {
//     static auto array_1 = create_array_cpp11<cell_constants_1>();
//     static auto array_2 = create_array_cpp11<cell_constants_2>();
//     static auto array_5 = create_array_cpp11<cell_constants_5>();
// }

void test2()
{
    static auto array_1 = create_array_cpp11_2<cell_constants_1>();
    static auto array_2 = create_array_cpp11_2<cell_constants_2>();
    static auto array_5 = create_array_cpp11_2<cell_constants_5>();
}

// void test3()
// {
//     static auto array_1 = table_initializer<cell_constants_1>::init();
//     static auto array_2 = table_initializer<cell_constants_2>::init();
//     static auto array_5 = table_initializer<cell_constants_5>::init();
// }


int main() {
    // auto array = create_array_cpp17<table_initializer>();
    // test1();
    // test2();
    // test3();
    return 0;
}