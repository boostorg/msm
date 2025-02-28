# Boost MSM optimizations

Most efficient improvements:

- Replacement of CPU-intensive calls due to C++03 recursion from MPL to Mp11
- Applied type punning where useful (to reduce template instantiations, std::deque & other things around the dispatch table)
- Optimized cell initialization with initializer arrays (to reduce template instantiations)
- favor_runtime_speed: Default-initialized everything and afterwards only defer transition cells
- favor_compile_time: "Default"-initialized only defer transition & call submachine cells


TODOs:

- Consider a way to put only events into the queue for favor_compile_time (boost::any)
- Consider a way to defer compilation of internal_start for favor_compile_time
- Add events to exit the sub-sms
- Consider preprocessing the whole stt in one go (only applies to favor compile time due to missing event hierarchy):
  - each row in the stt leads to one init cell constant, map with events as keys
  - Value: List of init_cell constants, where index relates to current_state_type and function needs to be deduced
- Consider trying out the tuple impl from SML
- Consider using a std::vector for the chain row (favor compile time)
- Understand the background of region_start_helper


Ideas short-term:

- defer the constructor of a fsm with favor_compile_time to a GENERATE macro
- Skip the dispatch table if the event is not part of the SM's transition table (non-recursive)
-> There are 3 options: Sub-SM is active (call it) or event is deferred (defer it) or have no action (call no transition)


Ideas long-term:

- Switch tables to have events as rows instead of states as rows
-> Would be another m_dispatch_tables member in state_machine
-> Needs remapping before dispatch:
--> Event gets converted to an index and used in a mapping table, that gets the actual index in the dispatch table of the current state
--> Then we can execute the function



Learnings:

- If only a subset needs to be processed, prefer copy_if & transform over fold
- Selecting a template-based function overload in Mp11 seems more efficient than using enable_if/disable_if