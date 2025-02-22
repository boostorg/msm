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

#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/advance.hpp>

#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/msm/event_traits.hpp>
#include <boost/msm/back/default_compile_policy.hpp>
#include <boost/msm/backmp11/metafunctions.hpp>
#include <boost/msm/back/common_types.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(is_frow)

namespace boost { namespace msm { namespace back 
{

template<typename CompilePolicy>
struct init_cell;

template<>
struct init_cell<favor_runtime_speed>
{
    // TODO:
    // Double-check function pointer casts, especially for args.
    typedef HandledEnum (*cell)();
    init_cell(cell* entries) : entries(entries) {}

    template<typename PreprocessedRow>
    void operator()(PreprocessedRow const&)
    {
        if (!mp11::mp_first<PreprocessedRow>::value)
        {
            entries[mp11::mp_second<PreprocessedRow>::value] = reinterpret_cast<cell>(mp11::mp_third<PreprocessedRow>::value);
        }
    }

    cell* entries;
};

template<typename CompilePolicy>
struct default_init_cell;

template<>
struct default_init_cell<favor_runtime_speed>
{
    // TODO:
    // Double-check function pointer casts, especially for args.
    typedef HandledEnum (*cell)();
    default_init_cell(cell* entries) : entries(entries) {}

    template<typename PreprocessedState>
    void operator()(PreprocessedState const&)
    {
        entries[mp11::mp_first<PreprocessedState>::value] = reinterpret_cast<cell>(mp11::mp_second<PreprocessedState>::value);
    }

    cell* entries;
};


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

    // Helpers for default_init_cell
    template<typename fsm = Fsm>
    using fsm_defer_transition = std::integral_constant<
        cell,
        &fsm::defer_transition
        >;
    template<typename State>
    using preprocess_state = mp11::mp_list<
        // Offset into the entries array
        mp11::mp_if_c<
            is_same<State,Fsm>::value,
            mp11::mp_size_t<0>,
            mp11::mp_size_t<get_state_id<Stt,State>::value + 1>
            >,
        // Address of the function to assign
        mp11::mp_if_c<
            is_completion_event<Event>::type::value,
            std::integral_constant<
                cell,
                &Fsm::default_eventless_transition
                >,
            mp11::mp_eval_if_c<
                !has_state_delayed_event<State,Event>::type::value,
                mp11::mp_if_c<
                    is_same<State,Fsm>::value,
                    std::integral_constant<
                        cell,
                        &Fsm::call_no_transition_internal
                        >,
                    std::integral_constant<
                        cell,
                        &Fsm::call_no_transition
                        >
                    >,
                fsm_defer_transition,
                Fsm
                >
            >
        >;

    // Helpers for first operation (fold)
    template <typename T>
    using event_filter_predicate = mp11::mp_or<
        is_base_of<typename T::transition_event, Event>,
        typename is_kleene_event<typename T::transition_event>::type
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
    
    // Helpers for second operation (transform)
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
    using preprocess_row_helper = std::integral_constant<
        cell,
        &Transition::execute
        >;
    template<typename Transition>
    using preprocess_row = mp11::mp_list<
        // Condition for executing
        has_not_real_row_tag<Transition>,
        // Offset into the entries array
        mp11::mp_if_c<
            is_same<typename Transition::current_state_type,Fsm>::value,
            mp11::mp_size_t<0>,
            mp11::mp_size_t<get_state_id<typename create_stt<Fsm>::type, typename Transition::current_state_type>::value + 1>
            >,
        // Address of the execute function
        mp11::mp_eval_if_c<
            is_kleene_event<typename Transition::transition_event>::type::value,
            std::integral_constant<
                cell,
                // TODO:
                // Try out against enable_if in convert_event_and_forward
                &convert_event_and_forward<Transition>::execute
                >,
            preprocess_row_helper,
            Transition
            >
        >;


 public:
    // initialize the dispatch table for a given Event and Fsm
    dispatch_table()
    {
        using default_init_cell = default_init_cell<favor_runtime_speed>;
        using init_cell = init_cell<favor_runtime_speed>;

        // Initialize cells for no transition
        typedef mp11::mp_transform<
            preprocess_state,
            typename generate_state_set<Stt>::state_set_mp11
            > preprocessed_states;
        mp11::mp_for_each<preprocessed_states>
            (default_init_cell{reinterpret_cast<default_init_cell::cell*>(entries)});
        
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
        mp11::mp_for_each<chained_and_preprocessed_rows>
            (init_cell{reinterpret_cast<init_cell::cell*>(entries)});
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

