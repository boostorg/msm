# Boost MSM optimizations

Major improvements:

- Replacement of CPU-intensive calls due to C++03 recursion from MPL to Mp11
- Applied type punning where useful (to reduce template instantiations, std::deque & other things around the dispatch table)
- favor_runtime_speed: Optimized cell initialization with initializer arrays (to reduce template instantiations)
  - check again if this was really helpful
- favor_runtime_speed: Default-initialized everything and afterwards only defer transition cells
- favor_compile_time: Use one dispatch table per state to reduce compiler processing time
- favor_compile_time: Apply type erasure with boost::any as early as possible and do further processing only with any events
  - dispatch tables are a hash tables with type_id as key
  - each dispatch table only has to cover the events it's handling, no template instantiations for forwarding events required
 - favor_compile_time: Use std::any if C++17 is available (up to 30% runtime impact because of small value optimization in std::any)


## TODOs:

- Consider trying out the tuple impl from SML
- Understand the background of region_start_helper

## Learnings:

- If only a subset needs to be processed, prefer copy_if & transform over fold
- Selecting a template-based function overload in Mp11 seems more efficient than using enable_if/disable_if
