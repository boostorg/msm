// Copyright 2010 Christophe Henry
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
#define BOOST_TEST_MODULE back11_orthogonal_deferred3_with_puml_test
#endif
#include <boost/test/unit_test.hpp>

using namespace std;
namespace msm = boost::msm;
using namespace msm::front;
using namespace msm::front::puml;

namespace {
    // Flags. Allow information about a property of the current state
    struct PlayingPaused {};
    struct CDLoaded {};
    struct FirstSongPlaying {};
}

namespace boost::msm::front::puml {
    // events
    template<>
    struct Event<by_name("NextSong")> {};
    template<>
    struct Event<by_name("PreviousSong")> {};
    // The list of FSM states
    template<>
    struct State<by_name("Song1")> : public msm::front::state<>
    {
        typedef boost::fusion::vector<FirstSongPlaying>      flag_list;

        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("Song2")> : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("Song3")> : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };

    //actions
    template<>
    struct Action<by_name("start_next_song")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.start_next_song_counter;
        }
    };
    template<>
    struct Action<by_name("start_prev_song")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    // guard conditions
    template<>
    struct Guard<by_name("start_prev_song_guard")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.start_prev_song_guard_counter;
            return true;
        }
    };
}

namespace
{
    // front-end: define the FSM structure 
    struct playing_ : public msm::front::state_machine_def<playing_>
    {
        typedef boost::fusion::vector<PlayingPaused, CDLoaded>        flag_list;

        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
        unsigned int start_next_song_counter = 0;
        unsigned int start_prev_song_guard_counter = 0;

        BOOST_MSM_PUML_DECLARE_TABLE(
            R"( 
            @startuml Player
            skinparam linetype polyline
            state Player{
                [*]-> Song1
                Song1     -> Song2   : NextSong         
                Song2     -> Song1   : PreviousSong   / start_prev_song [start_prev_song_guard]
                Song2     -> Song3   : NextSong       / start_next_song
                Song3     -> Song2   : PreviousSong                     [start_prev_song_guard]
            }
            @enduml
        )"
        )

        // Replaces the default no-transition response.
        template <class FSM, class Event>
        void no_transition(Event const&, FSM&, int)
        {
            BOOST_FAIL("no_transition called!");
        }
    };
}
namespace boost::msm::front::puml {
    template<>
    struct State<by_name("Playing")> : public msm::back11::state_machine<playing_>
    {

    };
}

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
    struct Event<by_name("error_found")> {};
    template<>
    struct Event<by_name("end_error")> {};
    template<>
    struct Event<by_name("do_terminate")> {};

    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD = 0,
        DISK_DVD = 1
    };
    template<>
    struct Event<by_name("cd_detected")>
    {
        Event(std::string name)
            : name(name)
        {}

        std::string name;
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
    }; 
    template<>
    struct State<by_name("Open")> : public msm::front::state<>
    {
        typedef boost::fusion::vector<CDLoaded>      flag_list;

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
        typedef boost::fusion::vector<CDLoaded>      flag_list;

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
        typedef boost::fusion::vector<PlayingPaused, CDLoaded>        flag_list;

        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("AllOk")> : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("ErrorMode")> : 
        public msm::front::interrupt_state < Event<by_name("end_error")>>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter = 0;
        int exit_counter = 0;
    };
    template<>
    struct State<by_name("ErrorTerminate")> : public msm::front::terminate_state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
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
    struct Action<by_name("close_drawer")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
        {
        }
    };
    template<>
    struct Action<by_name("TestFct")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
            void operator()(EVT& e, FSM& fsm,SourceState& ,TargetState& )
            {
                ++e.cpt_;
                ++fsm.test_fct_counter;
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
        void operator()(EVT const&, FSM&, SourceState&, TargetState&)
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
    struct Action<by_name("report_error")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.report_error_counter;
        }
    };
    template<>
    struct Action<by_name("report_end_error")>
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
        {
            ++fsm.report_end_error_counter;
        }
    };
    // guard conditions
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
}

namespace
{


    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        // we want deferred events and no state requires deferred events (only the fsm in the
        // transition table), so the fsm does.
        typedef int activate_deferred_events;

        unsigned int start_playback_counter=0;
        unsigned int can_close_drawer_counter=0;
        unsigned int report_error_counter=0;
        unsigned int report_end_error_counter=0;

        BOOST_MSM_PUML_DECLARE_TABLE(
            R"( 
            @startuml Player
            skinparam linetype polyline
            state Player{
                [*]-> Empty
                Stopped     -> Playing   : play          / start_playback
                Stopped     -> Open      : open_close   / open_drawer
                Stopped     -> Stopped   : stop

                Open        -> Empty     : open_close                                [can_close_drawer]
                Open        -> Open      : *play         / defer
                
                Empty       --> Open     : open_close    / open_drawer
                Empty       ---> Stopped : cd_detected   / store_cd_info
                Empty       -> Empty     : *play         / defer
                
                Playing     --> Stopped  : stop          / stop_playback
                Playing     -> Paused    : pause         / pause_playback
                Playing     --> Open     : open_close    / stop_and_open

                Paused      -> Playing   : end_pause     / resume_playback
                Paused      --> Stopped  : stop          / stop_playback
                Paused      --> Open     : open_close    / stop_and_open
                --
                [*]-> AllOk
                AllOk       -> ErrorMode : error_found / report_error
                ErrorMode   -> AllOk     : end_error   / report_end_error
                AllOk  -> ErrorTerminate : do_terminate
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


    BOOST_AUTO_TEST_CASE( back11_orthogonal_deferred3_with_pumltest )
    {
        player p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start(); 
        // test deferred event
        // deferred in Empty and Open, will be handled only after event cd_detected
        p.process_event(Event<by_name("play")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2, "Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Open")>&>().exit_counter == 0, "Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().entry_counter == 0, "Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().entry_counter == 1, "Empty entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<CDLoaded>() == false, "CDLoaded should not be active");
        p.process_event(Event<by_name("open_close")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1, "Open should be active"); //Open
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().exit_counter == 1, "Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Open")>&>().entry_counter == 1, "Open entry not called correctly");
        p.process_event(Event<by_name("open_close")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2, "Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Open")>&>().exit_counter == 1, "Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().entry_counter == 2, "Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.can_close_drawer_counter == 1, "guard not called correctly");
        
        //deferred event should have been processed
        p.process_event(Event<by_name("cd_detected")>{"louie, louie"});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Empty")>&>().exit_counter == 2, "Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 1, "Stopped entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().exit_counter == 1, "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().entry_counter == 1, "Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.start_playback_counter == 1, "action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().current_state()[0] == 0, "Song1 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().get_state<State<by_name("Song1")>&>().entry_counter == 1,
            "Song1 entry not called correctly");

        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == true, "PlayingPaused should be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<FirstSongPlaying>() == true, "FirstSongPlaying should be active");

        p.process_event(Event<by_name("NextSong")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().current_state()[0] == 1, "Song2 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().get_state<State<by_name("Song2")>&>().entry_counter == 1,
            "Song2 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().get_state<State<by_name("Song1")>&>().exit_counter == 1,
            "Song1 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().start_next_song_counter == 0,
            "submachine action not called correctly");

        p.process_event(Event<by_name("NextSong")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().current_state()[0] == 2, "Song3 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().get_state<State<by_name("Song3")>&>().entry_counter == 1,
            "Song3 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().get_state<State<by_name("Song2")>&>().exit_counter == 1,
            "Song2 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().start_next_song_counter == 1,
            "submachine action not called correctly");

        p.process_event(Event<by_name("PreviousSong")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().current_state()[0] == 1, "Song2 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().get_state<State<by_name("Song2")>&>().entry_counter == 2,
            "Song2 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().get_state<State<by_name("Song3")>&>().exit_counter == 1,
            "Song3 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<State<by_name("Playing")>&>().start_prev_song_guard_counter == 1,
            "submachine guard not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == true, "PlayingPaused should be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<FirstSongPlaying>() == false, "FirstSongPlaying should not be active");

        p.process_event(Event<by_name("pause")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4, "Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().exit_counter == 1, "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().entry_counter == 1, "Paused entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == true, "PlayingPaused should be active");

        // go back to Playing
        p.process_event(Event<by_name("end_pause")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().exit_counter == 1, "Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().entry_counter == 2, "Playing entry not called correctly");

        p.process_event(Event<by_name("pause")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4, "Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().exit_counter == 2, "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().entry_counter == 2, "Paused entry not called correctly");

        p.process_event(Event<by_name("stop")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0, "Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Paused")>&>().exit_counter == 2, "Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 2, "Stopped entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == false, "PlayingPaused should not be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<CDLoaded>() == true, "CDLoaded should be active");

        p.process_event(Event<by_name("stop")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0, "Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().exit_counter == 2, "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 3, "Stopped entry not called correctly");

        //test interrupt
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5, "AllOk should be active"); //AllOk
        p.process_event(Event<by_name("error_found")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 6, "ErrorMode should be active"); //ErrorMode
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("AllOk")>&>().exit_counter == 1, "AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("ErrorMode")>&>().entry_counter == 1, "ErrorMode entry not called correctly");
        // try generating more events
        p.process_event(Event<by_name("play")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 6, "ErrorMode should be active"); //ErrorMode
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("AllOk")>&>().exit_counter == 1, "AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("ErrorMode")>&>().entry_counter == 1, "ErrorMode entry not called correctly");
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0, "Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().exit_counter == 2, "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 3, "Stopped entry not called correctly");
        p.process_event(Event<by_name("end_error")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5, "AllOk should be active"); //AllOk
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("ErrorMode")>&>().exit_counter == 1, "ErrorMode exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("AllOk")>&>().entry_counter == 2, "AllOk entry not called correctly");
        p.process_event(Event<by_name("play")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().exit_counter == 3, "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().entry_counter == 3, "Playing entry not called correctly");

        //test terminate
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5, "AllOk should be active"); //AllOk
        p.process_event(Event<by_name("do_terminate")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7, "ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("AllOk")>&>().exit_counter == 2, "AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("ErrorTerminate")>&>().entry_counter == 1, "ErrorTerminate entry not called correctly");
        // try generating more events
        p.process_event(Event<by_name("stop")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7, "ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("ErrorTerminate")>&>().exit_counter == 0, "ErrorTerminate exit not called correctly");
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().exit_counter == 2, "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 3, "Stopped entry not called correctly");
        p.process_event(Event<by_name("end_error")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7, "ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().exit_counter == 2, "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 3, "Stopped entry not called correctly");
        p.process_event(Event<by_name("stop")>{});
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7, "ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3, "Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Playing")>&>().exit_counter == 2, "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<State<by_name("Stopped")>&>().entry_counter == 3, "Stopped entry not called correctly");

    }
}

