// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>

// back-end
#include <boost/msm/back11/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/functor_row.hpp>
// for And_ operator
#include <boost/msm/front/operator.hpp>
#ifndef BOOST_MSM_NONSTANDALONE_TEST
#define BOOST_TEST_MODULE back11_simple_with_functors_test
#endif
#include <boost/test/unit_test.hpp>

using namespace std;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

namespace
{
    // events
    struct play {int cpt_ = 0;};
    struct end_pause {int cpt_ = 0;};
    struct stop {};
    struct pause {};
    struct open_close {};

    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD=0,
        DISK_DVD=1
    };
    struct cd_detected
    {
        cd_detected(std::string name, DiskTypeEnum diskType)
            : name(name),
            disc_type(diskType)
        {}

        std::string name;
        DiskTypeEnum disc_type;
    };

    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        unsigned int start_playback_counter;
        unsigned int can_close_drawer_counter;
        unsigned int test_fct_counter;

        player_():
        start_playback_counter(0),
        can_close_drawer_counter(0),
        test_fct_counter(0)
        {}

        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };
        struct Open : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        // sm_ptr still supported but deprecated as functors are a much better way to do the same thing
        struct Stopped : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        struct Playing : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const& e,FSM& )
            {
                ++entry_counter;
                event_counter = e.cpt_;
            }
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
            int event_counter=0;
        };

        struct Paused : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        // the initial state of the player SM. Must be defined
        typedef Empty initial_state;

        // transition actions
        struct TestFct 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT& e, FSM& fsm,SourceState& ,TargetState& )
            {
                ++e.cpt_;
                ++fsm.test_fct_counter;
            }
        };
        struct start_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
            {
                ++fsm.start_playback_counter;
            }
        };
        struct open_drawer 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
            }
        };
        struct close_drawer 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
            }
        };
        struct store_cd_info 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const&,FSM& fsm ,SourceState& ,TargetState& )
            {
                fsm.process_event(play());
            }
        };
        struct stop_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
            }
        };
        struct pause_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
            }
        };
        struct resume_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT& e,FSM& ,SourceState& ,TargetState& )
            {
                ++e.cpt_;
            }
        };
        struct stop_and_open 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
            }
        };
        struct stopped_again 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
            }
        };
        // guard conditions
        struct DummyGuard 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const&,FSM&,SourceState&,TargetState&)
            {
                return true;
            }
        };
        struct good_disk_format 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT& evt ,FSM&,SourceState& ,TargetState& )
            {
                // to test a guard condition, let's say we understand only CDs, not DVD
                if (evt.disc_type != DISK_CD)
                {
                    return false;
                }
                return true;
            }
        };
        struct always_true 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const&  ,FSM&,SourceState& ,TargetState& )
            {             
                return true;
            }
        };
        struct can_close_drawer 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const&  ,FSM& fsm,SourceState& ,TargetState& )
            {      
                ++fsm.can_close_drawer_counter;
                return true;
            }
        };

        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : boost::fusion::vector<
            //    Start     Event         Next      Action               Guard
            //  +---------+-------------+---------+---------------------+----------------------+
            Row < Stopped , play        , Playing , ActionSequence_
                                                     <boost::fusion::vector<
                                                     TestFct,start_playback> >            
                                                                              , DummyGuard           >,
            Row < Stopped , open_close  , Open    , open_drawer               , none                 >,
            Row < Stopped , stop        , Stopped , none                      , none                 >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Open    , open_close  , Empty   , close_drawer              , can_close_drawer     >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Empty   , open_close  , Open    , open_drawer               , none                 >,
            Row < Empty   , cd_detected , Stopped , store_cd_info             , And_<good_disk_format,
                                                                                     always_true>    >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Playing , stop        , Stopped , stop_playback             , none                 >,
            Row < Playing , pause       , Paused  , pause_playback            , none                 >,
            Row < Playing , open_close  , Open    , stop_and_open             , none                 >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Paused  , end_pause   , Playing , resume_playback           , none                 >,
            Row < Paused  , stop        , Stopped , stop_playback             , none                 >,
            Row < Paused  , open_close  , Open    , stop_and_open             , none                 >

            //  +---------+-------------+---------+---------------------+----------------------+
        >/*>::type*/ {};
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const&, FSM&,int)
        {
            BOOST_FAIL("no_transition called!");
        }
        // init counters
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) 
        {
            fsm.template get_state<player_::Stopped&>().entry_counter=0;
            fsm.template get_state<player_::Stopped&>().exit_counter=0;
            fsm.template get_state<player_::Open&>().entry_counter=0;
            fsm.template get_state<player_::Open&>().exit_counter=0;
            fsm.template get_state<player_::Empty&>().entry_counter=0;
            fsm.template get_state<player_::Empty&>().exit_counter=0;
            fsm.template get_state<player_::Playing&>().entry_counter=0;
            fsm.template get_state<player_::Playing&>().exit_counter=0;
            fsm.template get_state<player_::Paused&>().entry_counter=0;
            fsm.template get_state<player_::Paused&>().exit_counter=0;
        }

    };
    // Pick a back-end
    typedef msm::back11::state_machine<player_> player;

//    static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused" };


    BOOST_AUTO_TEST_CASE( back11_simple_with_functors_test )
    {     
        player p;

        p.start(); 
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 1,"Empty entry not called correctly");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Open should be active"); //Open
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().exit_counter == 1,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().entry_counter == 1,"Open entry not called correctly");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().exit_counter == 1,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 2,"Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.can_close_drawer_counter == 1,"guard not called correctly");

        p.process_event(
            cd_detected("louie, louie",DISK_DVD));
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().exit_counter == 1,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 2,"Empty entry not called correctly");

        p.process_event(
            cd_detected("louie, louie",DISK_CD)); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().exit_counter == 2,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 1,"Stopped entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 1,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 1,"Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.start_playback_counter == 1,"action not called correctly");
        BOOST_CHECK_MESSAGE(p.test_fct_counter == 1,"action not called correctly");

        p.process_event(pause());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 1,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().entry_counter == 1,"Paused entry not called correctly");

        // go back to Playing
        p.process_event(end_pause());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().exit_counter == 1,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 2,"Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().event_counter == 1,"Playing event counter incorrect");

        p.process_event(pause()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().entry_counter == 2,"Paused entry not called correctly");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().exit_counter == 2,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 2,"Stopped entry not called correctly");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 2,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 3,"Stopped entry not called correctly");

    }
}

