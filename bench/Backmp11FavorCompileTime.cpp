#include "common.hpp"
#include <boost/msm/backmp11/state_machine.hpp>
#include <boost/msm/backmp11/favor_compile_time.hpp>

using fsm = msm::back::state_machine<fsm_, msm::back::favor_compile_time>;

int main()
{
    test_fsm<fsm>();

    return 0;
}