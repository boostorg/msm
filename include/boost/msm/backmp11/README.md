# Boost MSM optimizations

Major improvements:

- Replacement of CPU-intensive calls due to C++03 recursion from MPL to Mp11
- Applied type punning where useful (to reduce template instantiations, std::deque & other things around the dispatch table)
- Optimized cell initialization with initializer arrays (to reduce template instantiations)
- favor_runtime_speed: Default-initialized everything and afterwards only defer transition cells
- favor_compile_time: Default-initialized only defer transition & call submachine cells
- favor_compile_time: Optimized algorithm for transition table processing to process each row only once (see group_rows_by_event)

TODOs:

- Look into creating only one dispatch table to reduce template instantiations
- Consider a way to put only events into the queue
- favor_compile_time: Call submachines via process_event_internal directly instead of process_any_event
- Look into recursive processing of transition tables wrt. event and state sets
- Consider trying out the tuple impl from SML
- Understand the background of region_start_helper


Ideas long-term:

- Switch tables to have events as rows instead of states as rows
-> Would be another m_dispatch_tables member in state_machine
-> Needs remapping before dispatch:
--> Event gets converted to an index and used in a mapping table, that gets the actual index in the dispatch table of the current state
--> Then we can execute the function



Learnings:

- If only a subset needs to be processed, prefer copy_if & transform over fold
- Selecting a template-based function overload in Mp11 seems more efficient than using enable_if/disable_if