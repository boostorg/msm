// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <iostream>

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/core/argument.hpp>


#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>


using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;
using namespace boost::phoenix;

using boost::phoenix::arg_names::arg1;
using boost::phoenix::arg_names::arg2;
using boost::phoenix::arg_names::arg3;
using boost::phoenix::arg_names::arg4;
//namespace boost { namespace phoenix { namespace arg_names
//{
//    expression::argument<3>::type const arg4 = {};
//
//}} }
using boost::phoenix::arg_names::arg4;
// entry/exit/action/guard logging functors
#include "logging_functors.h"

namespace  // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_EVENT(open_close)

    // Concrete FSM implementation 
    // The list of FSM states

    struct Empty_impl : public msm::front::state<> , public euml_state<Empty_impl>
    {
        // this allows us to add some functions
        void activate_empty() {std::cout << "switching to Empty " << std::endl;}
        // standard entry behavior
        template <class Event,class FSM>
        void on_entry(Event const& evt,FSM& fsm) 
        {
            std::cout << "entering: Empty" << std::endl;
        }
        template <class Event,class FSM>
        void on_exit(Event const& evt,FSM& fsm) 
        {
            std::cout << "leaving: Empty" << std::endl;
        }
    };
    //instance for use in the transition table
    Empty_impl const Empty;

    // define more states
    BOOST_MSM_EUML_STATE(( Open_Entry,Open_Exit ),Open)

    struct some_guard_impl
    {
        typedef bool result_type;

        bool operator()()
        {
            cout << "calling: some_guard" << endl;
            return true;
        }
    };
    boost::phoenix::function<some_guard_impl> some_guard;
    
    struct some_action_impl
    {
        typedef void result_type;

        void operator()()
        {
            cout << "calling: some_action" << endl;
        }
    };
    boost::phoenix::function<some_action_impl> some_action;

    // replaces the old transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
          Open     == Empty     + open_close  [some_guard()&&some_guard()] / (some_action(),some_action())
          
          // these ones are also ok
          //Open     == Empty     + open_close  / some_action()
          //Empty     + open_close  [some_guard()] / some_action()
          //Open     == Empty   / some_action()
          //  +------------------------------------------------------------------------------+
         ),transition_table)

    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Empty, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << no_attributes_, // Attributes
                                        configure_ << no_configure_, // configuration
                                        Log_No_Transition // no_transition handler
                                        ),
                                      player_) //fsm name

    // or simply, if no no_transition handler needed:
    //BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
    //                                    Empty // Init State
    //                                 ),player_)

    // choice of back-end
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Paused", "Open", "Empty", "Playing" };
    void pstate(player const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

    void test()
    {        
        player p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start();
        // note that we write open_close and not open_close(), like usual. Both are possible with eUML, but 
        // you now have less to type.
        // go to Open, call on_exit on Empty, then action, then on_entry on Open
        p.process_event(open_close); pstate(p);
    }
}

int main()
{
    test();
    return 0;
}
