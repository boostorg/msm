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

#ifndef BOOST_MSM_BACKMP11_DETAIL_STATE_VISITOR_HPP
#define BOOST_MSM_BACKMP11_DETAIL_STATE_VISITOR_HPP

#include <boost/msm/backmp11/detail/dispatch_table.hpp>
#include <boost/msm/backmp11/detail/metafunctions.hpp>

namespace boost::msm::backmp11::detail
{

constexpr bool has_flag(visit_mode value, visit_mode flag)
{
    return (static_cast<int>(value) & static_cast<int>(flag)) != 0;
}

// Helper to apply multiple predicates sequentially.
template <typename List, template <typename> typename... Predicates>
struct copy_if_impl;
template <typename List, template <typename> typename Predicate, template <typename> typename... Predicates>
struct copy_if_impl<List, Predicate, Predicates...>
{
    using subset = mp11::mp_copy_if<List, Predicate>;
    using type = typename copy_if_impl<subset, Predicates...>::type;
};
template <typename List>
struct copy_if_impl<List>
{
    using type = List;
};
template <typename List, template <typename> typename... Predicates>
using copy_if = typename copy_if_impl<List, Predicates...>::type;

template <typename StateMachine, template <typename> typename... Predicates>
using state_subset = copy_if<typename StateMachine::internal::state_set, Predicates...>;

// State visitor implementation.
// States to visit can be selected with Mode
// and additionally compile-time filtered with Predicates.
template <
    typename StateMachine,
    typename Visitor,
    visit_mode Mode,
    typename Enable = void,
    template <typename> typename... Predicates
    >
class state_visitor_impl;

template <
    typename StateMachine,
    typename Visitor,
    visit_mode Mode,
    template <typename> typename... Predicates>
class state_visitor_impl<
    StateMachine,
    Visitor,
    Mode,
    std::enable_if_t<
        has_flag(Mode, visit_mode::active_states) &&
        mp11::mp_not<mp11::mp_empty<state_subset<StateMachine, Predicates...>>>::value>,
    Predicates...>
{
  public:
    static void visit(StateMachine& sm, Visitor visitor)
    {
        if (sm.m_running)
        {
            const state_visitor_impl& self = instance();
            for (const int state_id : sm.m_active_state_ids)
            {
                self.dispatch(sm, state_id, std::forward<Visitor>(visitor));
            }
        }
    }

    // Bug: Clang 17 complains about private member if this method is private.
    template <typename State>
    static void accept(StateMachine& sm, Visitor visitor)
    {
        auto& state = sm.template get_state<State>();
        visitor(state);

        if constexpr (has_state_machine_tag<State>::value &&
                      has_flag(Mode, visit_mode::recursive))
        {
            state.template visit_if<Mode, Predicates...>(std::forward<Visitor>(visitor));
        }
    }

  private:
    using state_set = typename StateMachine::internal::state_set;
    using cell_t = void (*)(StateMachine&, Visitor);
    template <size_t index, cell_t cell>
    using init_cell_constant = init_cell_constant<index, cell_t, cell>;

    template <typename State>
    using get_init_cell_constant = init_cell_constant<
        StateMachine::template get_state_id<State>(),
        &accept<State>>;

    state_visitor_impl()
    {
        using init_cell_constants = mp11::mp_transform<
            get_init_cell_constant,
            state_subset<StateMachine, Predicates...>>;
        dispatch_table_initializer::execute(
            reinterpret_cast<generic_cell*>(m_cells),
            reinterpret_cast<const generic_init_cell_value*>(value_array<init_cell_constants>),
            mp11::mp_size<init_cell_constants>::value);
    }

    void dispatch(StateMachine& sm, int state_id, Visitor visitor) const
    {
        const cell_t& cell = m_cells[state_id];
        if (cell)
        {
            (*cell)(sm, std::forward<Visitor>(visitor));
        }
    }

    static const state_visitor_impl& instance()
    {
        static const state_visitor_impl instance;
        return instance;
    }

    cell_t m_cells[mp11::mp_size<state_set>::value]{};
};

template <
    typename StateMachine,
    typename Visitor,
    visit_mode Mode,
    template <typename> typename... Predicates>
class state_visitor_impl<
    StateMachine,
    Visitor,
    Mode,
    std::enable_if_t<
        has_flag(Mode, visit_mode::all_states) &&
        mp11::mp_not<mp11::mp_empty<state_subset<StateMachine, Predicates...>>>::value>,
    Predicates...>
{
  public:
    static void visit(StateMachine& sm, Visitor visitor)
    {
        using state_identities =
            mp11::mp_transform<mp11::mp_identity,
                               state_subset<StateMachine, Predicates...>>;
        mp11::mp_for_each<state_identities>(
            [&sm, &visitor](auto state_identity)
            {
                using State = typename decltype(state_identity)::type;
                auto& state = sm.template get_state<State>();
                visitor(state);

                if constexpr (has_state_machine_tag<State>::value &&
                              has_flag(Mode, visit_mode::recursive))
                {
                    state.template visit_if<Mode, Predicates...>(std::forward<Visitor>(visitor));
                }
            }
        );
    }
};

template <
    typename StateMachine,
    typename Visitor,
    visit_mode Mode,
    template <typename> typename... Predicates>
class state_visitor_impl<
    StateMachine,
    Visitor,
    Mode,
    std::enable_if_t<
        mp11::mp_empty<state_subset<StateMachine, Predicates...>>::value>,
    Predicates...>
{
  public:
    static constexpr void visit(StateMachine&, Visitor)
    {
    }
};

template <
    typename StateMachine,
    typename Visitor,
    visit_mode Mode,
    template <typename> typename... Predicates>
using state_visitor = state_visitor_impl<
    StateMachine,
    Visitor,
    Mode,
    /*Enable=*/void,
    Predicates...>;

} // boost::msm::backmp11::detail

#endif // BOOST_MSM_BACKMP11_DETAIL_STATE_VISITOR_HPP
