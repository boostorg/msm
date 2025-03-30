# Boost MSM optimizations

Major improvements:

- Replacement of CPU-intensive calls due to C++03 recursion from MPL to Mp11
- Applied type punning where useful (to reduce template instantiations, std::deque & other things around the dispatch table)
- Optimized cell initialization with initializer arrays (to reduce template instantiations)
- favor_runtime_speed: Default-initialized everything and afterwards only defer transition cells
- favor_compile_time: Default-initialized only defer transition & call submachine cells
- favor_compile_time: Optimized algorithm for transition table processing to process each row only once (see group_rows_by_event)

## TODOs:

- Look into creating only one dispatch table to reduce template instantiations
- Consider a way to put only events into the queue
- favor_compile_time: Call submachines via process_event_internal directly instead of process_any_event
- Look into recursive processing of transition tables wrt. event and state sets
- Consider trying out the tuple impl from SML
- Understand the background of region_start_helper

## Learnings:

- If only a subset needs to be processed, prefer copy_if & transform over fold
- Selecting a template-based function overload in Mp11 seems more efficient than using enable_if/disable_if


# Other ideas to try out long-term:

## Switch tables to have events as rows instead of states as rows

Idea:
- Would be another m_dispatch_tables member in state_machine
- Needs remapping before dispatch:
-> Event gets converted to an index and used in a mapping table, that gets the actual index in the dispatch table of the current state
-> Then we can execute the function
- The index into the map should be given by key via hash map:
-> This way submachines can be called without having to create their dispatch tables (distributed compilation)


Expected benefits:
- Less dispatch tables (since they exist per state, there should be less states than events in general)
- processing for compiler should be less: The state_filter_predicate and state-specific preprocessings are not required anymore.


Challenges:
- Will be difficult to create init cells for the dispatch table, because each row will have a different kind of function pointer.
-> Structured binding? Tuple assignment?
- What happens if an event without transition table entry gets passed? We cannot preserve the type info!
-> the no_transition invocation should remain in the header, think about "generating" only the dispatch table(s) and not process_event_internal


Next steps:

**Get rid of template instances for process_event, that are only required for forwarding to submachines**
- Try to create a generic process_any_event function that can be used for call_submachine and which doesn't need O(n) order.

**Choose what to generate for CU distribution**
- the dispatch table of a FSM itself should be all that's needed then, process_event shouldn't need to be generated anymore.