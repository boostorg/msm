// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACK_DISPATCH_TABLE_H
#define BOOST_MSM_BACK_DISPATCH_TABLE_H

#include <cstdint>
#include <utility>

#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/advance.hpp>

#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/msm/event_traits.hpp>
#include <boost/msm/back/traits.hpp>
#include <boost/msm/back/default_compile_policy.hpp>
#include <boost/msm/backmp11/metafunctions.hpp>
#include <boost/msm/back/common_types.hpp>

namespace boost { namespace msm { namespace back 
{

// Value used to initialize a cell of the dispatch table
template<typename Cell>
struct init_cell_value
{
    size_t index;
    Cell address;
};
template<size_t v1, typename Cell, Cell v2>
struct init_cell_constant
{
    static constexpr init_cell_value<Cell> value = {v1, v2};
};

// Type-punned init cell value to suppress redundant template instantiations
typedef HandledEnum (*generic_cell)();
struct generic_init_cell_value
{
    size_t index;
    generic_cell address;
};

// Helper to create an array of init cell values for table initialization
template<typename Cell, typename InitCellConstants, std::size_t... I>
static const init_cell_value<Cell>* const get_init_cells_impl(mp11::index_sequence<I...>)
{
    static constexpr init_cell_value<Cell> values[] {mp11::mp_at_c<InitCellConstants, I>::value...};
    return values;
}
template<typename Cell, typename InitCellConstants>
static const generic_init_cell_value* const get_init_cells()
{
    return reinterpret_cast<const generic_init_cell_value*>(
        get_init_cells_impl<Cell, InitCellConstants>(mp11::make_index_sequence<mp11::mp_size<InitCellConstants>::value>{}));
}

template<typename CompilePolicy>
struct cell_initializer;

template<>
struct cell_initializer<favor_runtime_speed>
{
    static void init(generic_cell* entries, const generic_init_cell_value* array, size_t size)
    {
        for (size_t i=0; i<size; i++)
        {
            const auto& item = array[i];
            entries[item.index] = item.address;
        }
    }
};

template<typename Fsm, typename State, typename Event>
struct table_index
{
    using type = mp11::mp_if<
        mp11::mp_or<
            mp11::mp_not<is_same<State, Fsm>>,
            typename has_state_delayed_event<State, Event>::type
            >,
        mp11::mp_size_t<get_state_id<typename create_stt<Fsm>::type, State>::value + 1>,
        mp11::mp_size_t<0>
        >;
};
template<typename Fsm, typename State>
struct table_index<Fsm, State, void>
{
    using type = mp11::mp_if<
        mp11::mp_not<is_same<State, Fsm>>,
        mp11::mp_size_t<get_state_id<typename create_stt<Fsm>::type, State>::value + 1>,
        mp11::mp_size_t<0>
        >;
};
template<typename Fsm, typename State, typename Event = void>
using get_table_index = typename table_index<Fsm, State, Event>::type;

// TODO:
// Check if a more generic dispatch table makes sense.
template <class Fsm,class Stt,class CompilePolicy>
struct new_dispatch_table;

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the SM take its transition on the given
// Event type.
template <class Fsm,class Stt, class Event,class CompilePolicy>
struct dispatch_table
{
 private:
    // This is a table of these function pointers.
    typedef HandledEnum (*cell)(Fsm&, int,int,Event const&);
    typedef bool (*guard)(Fsm&, Event const&);

    // class used to build a chain (or sequence) of transitions for a given event and start state
    // (like an UML diamond). Allows transition conflicts.
    template< typename Seq,typename AnEvent,typename State >
    struct chain_row
    {
        typedef State   current_state_type;
        typedef AnEvent transition_event;

        // helper for building a disable/enable_if-controlled execute function
        struct execute_helper
        {
            template <class Sequence>
            static
            HandledEnum
            execute(Fsm& , int, int, Event const& , ::boost::mpl::true_ const & )
            {
                // if at least one guard rejected, this will be ignored, otherwise will generate an error
                return HANDLED_FALSE;
            }

            template <class Sequence>
            static
            HandledEnum
            execute(Fsm& fsm, int region_index , int state, Event const& evt,
                    ::boost::mpl::false_ const & )
            {
                 // try the first guard
                 typedef typename ::boost::mpl::front<Sequence>::type first_row;
                 HandledEnum res = first_row::execute(fsm,region_index,state,evt);
                 if (HANDLED_TRUE!=res && HANDLED_DEFERRED!=res)
                 {
                    // if the first rejected, move on to the next one
                    HandledEnum sub_res = 
                         execute<typename ::boost::mpl::pop_front<Sequence>::type>(fsm,region_index,state,evt,
                            ::boost::mpl::bool_<
                                ::boost::mpl::empty<typename ::boost::mpl::pop_front<Sequence>::type>::type::value>());
                    // if at least one guards rejects, the event will not generate a call to no_transition
                    if ((HANDLED_FALSE==sub_res) && (HANDLED_GUARD_REJECT==res) )
                        return HANDLED_GUARD_REJECT;
                    else
                        return sub_res;
                 }
                 return res;
            }
        };
        // Take the transition action and return the next state.
        static HandledEnum execute(Fsm& fsm, int region_index, int state, Event const& evt)
        {
            // forward to helper
            return execute_helper::template execute<Seq>(fsm,region_index,state,evt,
                ::boost::mpl::bool_< ::boost::mpl::empty<Seq>::type::value>());
        }
    };
    // nullary metafunction whose only job is to prevent early evaluation of _1
    template< typename Entry > 
    struct make_chain_row_from_map_entry
    { 
        // if we have more than one frow with the same state as source, remove the ones extra
        // note: we know the frow's are located at the beginning so we remove at the beginning (number of frows - 1) elements
        enum { number_frows = boost::mp11::mp_count_if<typename Entry::second, has_is_frow>::value };

        //erases the first NumberToDelete rows
        template<class Sequence, int NumberToDelete>
        struct erase_first_rows
        {
            typedef typename ::boost::mpl::erase<
                typename Entry::second,
                typename ::boost::mpl::begin<Sequence>::type,
                typename ::boost::mpl::advance<
                        typename ::boost::mpl::begin<Sequence>::type, 
                        ::boost::mpl::int_<NumberToDelete> >::type
            >::type type;
        };
        // if we have more than 1 frow with this event (not allowed), delete the spare
        typedef typename ::boost::mpl::eval_if<
            typename ::boost::mpl::bool_< number_frows >= 2 >::type,
            erase_first_rows<typename Entry::second,number_frows-1>,
            ::boost::mpl::identity<typename Entry::second>
        >::type filtered_stt;

        typedef chain_row<filtered_stt,Event,
            typename Entry::first > type;
    }; 
    // helper for lazy evaluation in eval_if of change_frow_event
    template <class Transition,class NewEvent>
    struct replace_event
    {
        typedef typename Transition::template replace_event<NewEvent>::type type;
    };
    // changes the event type for a frow to the event we are dispatching
    // this helps ensure that an event does not get processed more than once because of frows and base events.
    template <class FrowTransition>
    struct change_frow_event
    {
        typedef typename ::boost::mp11::mp_if_c<
            has_is_frow<FrowTransition>::type::value,
            replace_event<FrowTransition,Event>,
            boost::mp11::mp_identity<FrowTransition>
        >::type type;
    };
    // Compute the maximum state value in the sm so we know how big
    // to make the table
    typedef typename generate_state_set<Stt>::state_set_mp11 state_set_mp11;
    BOOST_STATIC_CONSTANT(int, max_state = (mp11::mp_size<state_set_mp11>::value));

    template <class Transition>
    struct convert_event_and_forward
    {
        static HandledEnum execute(Fsm& fsm, int region_index, int state, Event const& evt)
        {
            typename Transition::transition_event forwarded(evt);
            return Transition::execute(fsm,region_index,state,forwarded);
        }
    };

    using init_cell_value = init_cell_value<cell>;

    template<size_t v1, cell v2>
    using init_cell_constant = init_cell_constant<v1, cell, v2>;

    template<cell v>
    using cell_constant = std::integral_constant<cell, v>;

    using cell_initializer = cell_initializer<favor_runtime_speed>;

    // Helpers for state processing
    template<typename State>
    using state_filter_predicate = typename has_state_delayed_event<State, Event>::type;
    template<typename State, typename fsm=Fsm>
    using preprocess_state = init_cell_constant<get_table_index<Fsm, State, Event>::value, &fsm::defer_transition>;

    // Helpers for row processing
    // First operation (fold)
    template <typename T>
    using event_filter_predicate = mp11::mp_and<
        mp11::mp_not<has_not_real_row_tag<T>>,
        mp11::mp_or<
            is_base_of<typename T::transition_event, Event>,
            typename is_kleene_event<typename T::transition_event>::type
            >
        >;
    template <typename M, typename Key, typename Value>
    using push_map_value = mp11::mp_push_front<
        mp11::mp_second<mp11::mp_map_find<M, Key>>,
        Value>;
    template<typename M, typename T>
    using map_updater = mp11::mp_map_replace<
        M,
        mp11::mp_list<
            typename T::current_state_type,
            mp11::mp_eval_if_c<
                !mp11::mp_map_contains<M, typename T::current_state_type>::value,
                // first row on this source state, make a list with 1 element
                mp11::mp_list<typename change_frow_event<T>::type>,
                // list already exists, add the row
                push_map_value,
                M,
                typename T::current_state_type,
                typename change_frow_event<T>::type
                >
            >
        >;
    // Second operation (transform)
    template<typename T>
    using to_mpl_map_entry = mpl::pair<
        mp11::mp_first<T>,
        mp11::mp_second<T>
        >;
    template<typename T>
    using row_chainer = mp11::mp_if_c<
        (mp11::mp_size<typename to_mp_list<mp11::mp_second<T>>::type>::value > 1),
        // we need row chaining
        typename make_chain_row_from_map_entry<to_mpl_map_entry<T>>::type,
        // just one row, no chaining, we rebuild the row like it was before
        mp11::mp_front<mp11::mp_second<T>>
        >;
    template<typename Transition>
    using preprocess_row_helper = cell_constant<&Transition::execute>;
    template<typename Transition>
    using preprocess_row = init_cell_constant<
        // Offset into the entries array
        get_table_index<Fsm, typename Transition::current_state_type>::value,
        // Address of the execute function
        mp11::mp_eval_if_c<
            is_kleene_event<typename Transition::transition_event>::type::value,
            cell_constant<
                // TODO:
                // Try out against enable_if in convert_event_and_forward
                &convert_event_and_forward<Transition>::execute
                >,
            preprocess_row_helper,
            Transition
            >::value
        >;


 public:
    // initialize the dispatch table for a given Event and Fsm
    dispatch_table()
    {
        // Initialize cells for no transition
        for (size_t i=0;i<max_state+1; i++)
        {
            entries[i] = &Fsm::call_no_transition;
        }
        // Initialize cells for defer transition
        typedef mp11::mp_copy_if<
            typename generate_state_set<Stt>::state_set_mp11,
            state_filter_predicate
            > filtered_states;
        typedef mp11::mp_transform<
            preprocess_state,
            filtered_states
            > preprocessed_states;
        cell_initializer::init(
            reinterpret_cast<generic_cell*>(entries),
            get_init_cells<cell, preprocessed_states>(),
            mp11::mp_size<preprocessed_states>::value
            );
        
        // build chaining rows for rows coming from the same state and the current event
        // first we build a map of sequence for every source
        // in reverse order so that the frow's are handled first (UML priority)
        typedef mp11::mp_fold<
            mp11::mp_copy_if<
                typename to_mp_list<Stt>::type,
                event_filter_predicate
                >,
            mp11::mp_list<>,
            map_updater
            > map_of_row_seq;
        // and then build chaining rows for all source states having more than 1 row
        typedef mp11::mp_transform<
            row_chainer,
            map_of_row_seq
            > chained_rows;
        typedef mp11::mp_transform<
            preprocess_row,
            chained_rows
            > chained_and_preprocessed_rows;
        // Go back and fill in cells for matching transitions.
        cell_initializer::init(
            reinterpret_cast<generic_cell*>(entries),
            get_init_cells<cell, chained_and_preprocessed_rows>(),
            mp11::mp_size<chained_and_preprocessed_rows>::value
            );
    }

    // The singleton instance.
    static const dispatch_table& instance() {
        static dispatch_table table;
        return table;
    }

 public: // data members
     // +1 => 0 is reserved for this fsm (internal transitions)
    cell entries[max_state+1];
};

}}} // boost::msm::back


#endif //BOOST_MSM_BACK_DISPATCH_TABLE_H

