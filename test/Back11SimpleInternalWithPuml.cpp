// Copyright 2024 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


// back-end
#include <boost/msm/back11/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/puml/puml.hpp>

#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE back11_simple_internal_with_puml
#endif
#include <boost/test/unit_test.hpp>

using namespace std;
namespace msm = boost::msm;
using namespace msm::front;
using namespace msm::front::puml;

namespace boost::msm::front::puml {
    // events
    template<>
    struct Event<by_name("play")>  {};
    template<>
    struct Event<by_name("end_pause")>  {};
    template<>
    struct Event<by_name("stop")>  {};
    template<>
    struct Event<by_name("pause")>  {};
    template<>
    struct Event<by_name("open_close")>  {};
    template<>
    struct Event<by_name("internal_evt")> {};
    template<>
    struct Event<by_name("to_ignore")> {};
    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD = 0,
        DISK_DVD = 1
    };
    template<>
    struct Event<by_name("cd_detected")>
    {
        Event(std::string name, DiskTypeEnum diskType)
            : name(name),
            disc_type(diskType)
        {}

        std::string name;
        DiskTypeEnum disc_type;
    };

    // The list of FSM states
    template<>
    struct State<by_name("Empty")> : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {++entry_counter;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {++exit_counter;}
        int entry_counter=0;
        int exit_counter=0;
        unsigned int empty_internal_guard_counter=0;
        unsigned int empty_internal_action_counter=0;
    }; 
    template<>
    struct State<by_name("Open")> : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("Stopped")> : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("Paused")> : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("Playing")> : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const& e,FSM& ){++entry_counter;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {++exit_counter;}
        int entry_counter = 0;
        int exit_counter = 0;
    };

    //actions
    template<>
    struct Action<by_name("open_drawer")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("start_playback")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.start_playback_counter;
        }
    };
    template<>
    struct Action<by_name("store_cd_info")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("stop_playback")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("pause_playback")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("resume_playback")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("stop_and_open")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("stopped_again")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("internal_action")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.internal_action_counter;
        }
    };
    template<>
    struct Action<by_name("internal_action_fct")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState& src, TargetState&)
        {
            ++src.empty_internal_action_counter;
        }
    };
    // guard conditions
    template<>
    struct Guard<by_name("good_disk_format")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT& evt, FSM&, SourceState&, TargetState&)
        {
            // to test a guard condition, let's say we understand only CDs, not DVD
            if (evt.disc_type != DISK_CD)
            {
                return false;
            }
            return true;
        }
    };
    template<>
    struct Guard<by_name("can_close_drawer")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.can_close_drawer_counter;
            return true;
        }
    };
    template<>
    struct Guard<by_name("internal_guard")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.internal_guard_counter;
            return false;
        }
    };
    template<>
    struct Guard<by_name("internal_guard_fct")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const&, FSM&, SourceState& src, TargetState&)
        {
            ++src.empty_internal_guard_counter;
            return false;
        }
    };
    template<>
    struct Guard<by_name("internal_guard2")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.internal_guard_counter;
            return true;
        }
    };
}

namespace
{


    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        unsigned int start_playback_counter=0;
        unsigned int can_close_drawer_counter=0;
        unsigned int internal_action_counter=0;
        unsigned int internal_guard_counter=0;

        BOOST_MSM_PUML_DECLARE_TABLE(
            R"( 
            @startuml Player
            skinparam linetype polyline
            state Player{
                [*]-> Empty
                Stopped     -> Playing   : play         / start_playback
                Stopped     -> Open      : open_close   / open_drawer
                Stopped     -> Stopped   : stop

                Open        -> Empty     : open_close                             [can_close_drawer]
                
                Empty       --> Open     : open_close    / open_drawer
                Empty       ---> Stopped : cd_detected   / store_cd_info          [good_disk_format]
                Empty       -> Empty     : -internal_evt / internal_action        [internal_guard2]              
                Empty       -> Empty     : -to_ignore
                Empty       -> Empty     : -cd_detected                           [internal_guard]              
                Empty       -> Empty     : -internal_evt / internal_action_fct    [internal_guard_fct]              

                Playing     --> Stopped  : stop          / stop_playback
                Playing     -> Paused    : pause         / pause_playback
                Playing     --> Open     : open_close    / stop_and_open
                
                Paused      -> Playing   : end_pause     / resume_playback
                Paused      --> Stopped  : stop          / stop_playback
                Paused      --> Open     : open_close    / stop_and_open
            }
            @enduml
        )"
        )

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const&, FSM&,int)
        {
            BOOST_FAIL("no_transition called!");
        }
    };
    // Pick a back-end
    typedef msm::back11::state_machine<player_> player;


    BOOST_AUTO_TEST_CASE( back11_simple_internal_with_puml_test )
    {     
        player p;

        p.start(); 
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().entry_counter == 1, "Empty entry not called correctly");
        // internal events
        p.process_event(Event<by_name("to_ignore")>{});
        p.process_event(Event<by_name("internal_evt")>{});
        BOOST_CHECK_MESSAGE(p.internal_action_counter == 1, "Internal action not called correctly");
        BOOST_CHECK_MESSAGE(p.internal_guard_counter == 1, "Internal guard not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().empty_internal_action_counter == 0, "Empty internal action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().empty_internal_guard_counter == 1, "Empty internal guard not called correctly");

        p.process_event(Event<by_name("open_close")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Open should be active"); //Open
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().exit_counter == 1,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Open")>&>().entry_counter == 1,"Open entry not called correctly");

        p.process_event(Event<by_name("open_close")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Open")>&>().exit_counter == 1,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().entry_counter == 2,"Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.can_close_drawer_counter == 1,"guard not called correctly");

        p.process_event(Event<by_name("cd_detected")>{"louie, louie", DISK_DVD});

        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Open")>&>().exit_counter == 1,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().entry_counter == 2,"Empty entry not called correctly");

        p.process_event(Event<by_name("cd_detected")>{"louie, louie", DISK_CD});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0, "Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().exit_counter == 2,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 1,"Stopped entry not called correctly");
        BOOST_CHECK_MESSAGE(p.internal_guard_counter == 3, "Internal guard not called correctly");

        p.process_event(Event<by_name("play")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().exit_counter == 1, "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().entry_counter == 1, "Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.start_playback_counter == 1, "action not called correctly");

        p.process_event(Event<by_name("pause")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4, "Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().exit_counter == 1, "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().entry_counter == 1, "Paused entry not called correctly");

        // go back to Playing
        p.process_event(Event<by_name("end_pause")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().exit_counter == 1,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().entry_counter == 2,"Playing entry not called correctly");

        p.process_event(Event<by_name("pause")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().exit_counter == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().entry_counter == 2,"Paused entry not called correctly");

        p.process_event(Event<by_name("stop")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().exit_counter == 2,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 2,"Stopped entry not called correctly");

        p.process_event(Event<by_name("stop")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().exit_counter == 2,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 3,"Stopped entry not called correctly");

    }
}

