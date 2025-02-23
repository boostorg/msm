#define TRANSITION_TABLE_TYPE mpl::vector
#include "common.hpp"
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/favor_compile_time.hpp>

using fsm0 = msm::back::state_machine<fsm_, msm::back::favor_compile_time>;

int main()
{
    test_fsm<fsm0>();

    return 0;
}