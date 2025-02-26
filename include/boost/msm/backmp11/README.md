# Boost MSM optimizations

Most efficient improvements:

- Replacement of CPU-intensive calls due to C++03 recursion from MPL to Mp11
- Applied type punning where useful (to reduce template instantiations, std::deque & other things around the dispatch table)
- Optimized cell initialization with initializer arrays (to reduce template instantiations)
- favor_runtime_speed: Default-initialized everything and afterwards only defer transition cells
- favor_compile_time: "Default"-initialized only defer transition & call submachine cells


TODOs:

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