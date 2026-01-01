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

#include <boost/msm/backmp11/detail/metafunctions.hpp>

namespace boost::msm::backmp11::detail
{

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
template<typename List, template <typename> typename... Predicates>
using copy_if = typename copy_if_impl<List, Predicates...>::type;

template<typename StateMachine, template <typename> typename... Predicates>
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
    state_visitor_impl()
    {
        using state_identities =
            mp11::mp_transform<mp11::mp_identity,
                               state_subset<StateMachine, Predicates...>>;
        mp11::mp_for_each<state_identities>(
            [this](auto state_identity)
            {
                using State = typename decltype(state_identity)::type;
                m_cells[StateMachine::template get_state_id<State>()] = &accept<State>;
            }
        );
    }

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

  private:
    using state_set = typename StateMachine::internal::state_set;
    using cell_t = void (*)(StateMachine&, Visitor);

    template<typename State>
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

    static const state_visitor_impl& instance()
    {
        static const state_visitor_impl instance;
        return instance;
    }

    void dispatch(StateMachine& sm, int state_id, Visitor visitor) const
    {
        cell_t cell = m_cells[state_id];
        if (cell)
        {
            (*cell)(sm, std::forward<Visitor>(visitor));
        }
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
using state_visitor = state_visitor_impl<StateMachine, Visitor, Mode, void, Predicates...>;

} // boost::msm::backmp11::detail

#endif // BOOST_MSM_BACKMP11_DETAIL_STATE_VISITOR_HPP
