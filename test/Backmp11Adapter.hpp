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

#include "boost/msm/back/state_machine.hpp"
#include "boost/msm/backmp11/state_machine_config.hpp"
#include "boost/msm/backmp11/favor_compile_time.hpp"
#include "boost/msm/backmp11/state_machine.hpp"
#include "boost/msm/back/queue_container_deque.hpp"
#define BOOST_PARAMETER_CAN_USE_MP11
#include <boost/parameter.hpp>
#include <boost/serialization/array.hpp>
#include "Backmp11.hpp"

namespace boost::msm::backmp11
{

template <typename Archive>
class serializer
{
  public:
    serializer(Archive& archive) : m_archive(archive) {}

    template <typename FrontEnd>
    void visit_front_end(FrontEnd&& front_end)
    {
        if constexpr (has_serialize_member<T>::value)
        {
            m_archive & front_end;
        }
    }

    template <typename FrontEnd, typename Reflect>
    void visit_front_end(FrontEnd&& /*front_end*/, Reflect&& reflect)
    {
        reflect();
    }

    template <typename Member>
    void visit_member(const char* /*key*/, Member&& member)
    {
        m_archive & member;
    }

    template <typename State>
    void visit_state(size_t /*state_id*/, State&& state)
    {
        if constexpr (has_serialize_member<T>::value)
        {
            m_archive & state;
        }
    }

    template <typename State, typename Reflect>
    void visit_state(size_t /*state_id*/, State&& /*state*/, Reflect&& reflect)
    {
        reflect();
    }

  private:
    template <typename State, typename = void>
    struct has_serialize_member : std::false_type {};
    template <typename State>
    struct has_serialize_member<
        State,
        std::void_t<decltype(std::declval<State&>().serialize(
            std::declval<Archive&>(), 0u))>>
        : std::true_type {};

    Archive& m_archive;
};

using back::queue_container_deque;

template <
    class A1,
    class A2,
    class A3,
    class A4
>
struct state_machine_config_adapter : state_machine_config
{
    typedef ::boost::parameter::parameters<
      ::boost::parameter::optional<
            ::boost::parameter::deduced< back::tag::history_policy>, has_history_policy< ::boost::mpl::_ >
        >
    , ::boost::parameter::optional<
            ::boost::parameter::deduced< back::tag::compile_policy>, has_compile_policy< ::boost::mpl::_ >
        >
    , ::boost::parameter::optional<
            ::boost::parameter::deduced< back::tag::fsm_check_policy>, has_fsm_check< ::boost::mpl::_ >
        >
    , ::boost::parameter::optional<
            ::boost::parameter::deduced< back::tag::queue_container_policy>,
            has_queue_container_policy< ::boost::mpl::_ >
        >
    > config_signature;

    typedef typename
        config_signature::bind<A1,A2,A3,A4>::type
        config_args;

    typedef typename ::boost::parameter::binding<
        config_args, back::tag::compile_policy, favor_runtime_speed >::type    CompilePolicy;
    using compile_policy = mp11::mp_if_c<
        std::is_same_v<CompilePolicy, favor_runtime_speed>,
        favor_runtime_speed,
        favor_compile_time>;
    

    typedef typename ::boost::parameter::binding<
        config_args, back::tag::queue_container_policy,
        queue_container_deque >::type                                    QueueContainerPolicy;
    template<typename T>
    using queue_container = typename QueueContainerPolicy::template In<T>::type;
};

template <class A0,
          class A1 = parameter::void_,
          class A2 = parameter::void_,
          class A3 = parameter::void_,
          class A4 = parameter::void_>
class state_machine_adapter
    : public state_machine<A0, state_machine_config_adapter<A1, A2, A3, A4>, state_machine_adapter<A0, A1, A2, A3, A4>>
{
    using Base = state_machine<A0, state_machine_config_adapter<A1, A2, A3, A4>, state_machine_adapter<A0, A1, A2, A3, A4>>;
  public:
    using Base::Base;

    // The new API returns a const std::array<...>&.
    const uint16_t* current_state() const
    {
        return &this->get_active_state_ids()[0];
    }

    // The history can be accessed like this,
    // but it has to be configured in the front-end.
    auto& get_history()
    {
        return this->m_history;
    }

    auto& get_message_queue()
    {
        return this->get_event_pool().events;
    }

    size_t get_message_queue_size() const
    {
        return this->get_event_pool().events.size();
    }

    void execute_queued_events()
    {
        this->process_event_pool();
    }

    void execute_single_queued_event()
    {
        this->process_event_pool(1);
    }

    auto& get_deferred_queue()
    {
        return this->get_event_pool().events;
    }

    void clear_deferred_queue()
    {
        this->get_event_pool().events.clear();
    }

    template <class Archive>
    void serialize(Archive& ar,
                   const unsigned int /*version*/)
    {
        backmp11::reflect(*this, serializer<Archive>{ar});
    }

    // No adapter.
    // Superseded by the visitor API.
    // void visit_current_states(...) {...}

    // No adapter.
    // States can be set with `get_state<...>() = ...` or the visitor API.
    // void set_states(...) {...}

    // No adapter.
    // Could be implemented with the visitor API.
    // auto get_state_by_id(int id) {...}
};

} // boost::msm::backmp11
