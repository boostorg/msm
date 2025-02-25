# Boost MSM optimizations

Most efficient improvements:

- Replacement of CPU-intensive calls due to C++03 recursion from MPL to Mp11
- Applied type punning where useful (to reduce template instantiations, std::deque & other things around the dispatch table)
- Optimized cell initialization with initializer arrays (to reduce template instantiations)
- favor_runtime_speed: Default-initialized everything and afterwards only defer transition cells
- favor_compile_time: "Default"-initialized only defer transition & call submachine cells


TODOs:

- Get rid of one more fold (-> copy_if & transform) in dispatch_table
- Introduce cell_initializer in favor_compile_time


Ideas short-term:

- Skip the dispatch table if the event is not part of the SM's transition table

Ideas long-term:

- Switch tables to have events as rows instead of states as rows


Learnings:

- If only a subset needs to be processed, prefer copy_if & transform over fold
