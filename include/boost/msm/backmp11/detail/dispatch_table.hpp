// Copyright 2025 Christian Granzin
// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACKMP11_DISPATCH_TABLE_H
#define BOOST_MSM_BACKMP11_DISPATCH_TABLE_H

#include <cstddef>

#include <boost/mp11.hpp>
#include <boost/msm/backmp11/common_types.hpp>

namespace boost { namespace msm { namespace backmp11
{

namespace detail
{

// Value used to initialize a cell of the dispatch table
template<typename Cell>
struct init_cell_value
{
    size_t index;
    Cell cell;
};
template<size_t index, typename Cell, Cell cell>
struct init_cell_constant
{
    using value_type = init_cell_value<Cell>;
    static constexpr value_type value = {index, cell};
};

// Type-punned init cell value to suppress redundant template instantiations.
using generic_cell = void(*)();
using generic_init_cell_value = init_cell_value<generic_cell>;

// Class that handles the initialization of dispatch table entries.
struct dispatch_table_initializer
{
    static void execute(generic_cell* cells, const generic_init_cell_value* values, size_t values_size)
    {
        for (size_t i = 0; i < values_size; i++)
        {
            const auto& item = values[i];
            cells[item.index] = item.cell;
        }
    }
};

} // detail

}}} // boost::msm::backmp11

#endif //BOOST_MSM_BACKMP11_DISPATCH_TABLE_H
