# Boost MSM backmp11 backend

The new backend `backmp11` is a new backwards-compatible backend that has the following goals:

- reduce compilation runtime and RAM usage
- reduce state machine runtime

It is named after the metaprogramming library Boost Mp11, the main contributor to the optimizations:
It replaces usages of MPL with Mp11 to get rid of the C++03 emulation of variadic templates.
This backend contains additional optimizations that are further described below.


## How to use it

The backend and both its policies `favor_runtime_speed` and `favor_compile_time` should be compatible with existing code. Required replacements to try it out:
- use `boost::msm::backmp11::state_machine` in place of `boost::msm::back::state_machine` and
- use `boost::msm::backmp11::favor_compile_time` in place of `boost::msm::back::favor_compile_time`

When using the `favor_compile_time` policy, a different macro to generate missing parts of a SM is needed:
- use `BOOST_MSM_BACKMP11_GENERATE_DISPATCH_TABLE(<fsmname>)` in place of `BOOST_MSM_BACK_GENERATE_PROCESS_EVENT(<fsmname>)`


## General optimizations

- Replacement of CPU-intensive calls due to C++03 recursion from MPL to Mp11
- Applied type punning where useful (to reduce template instantiations, e.g. std::deque & other things around the dispatch table)


## Optimizations applied to the `favor_runtime_speed` policy

Summary:
- Optimized cell initialization with initializer arrays (to reduce template instantiations)
- Default-initialized everything and afterwards only defer transition cells


## Optimizations applied to the `favor_compile_time` policy

Once an event is given to the FSM for processing, it is immediately converted to `any` and processing continues with this `any` event.
The structure of the dispatch table has been reworked, one dispatch table is created per state as a hash map.
The state dispatch tables are designed to directly work with the `any` event, they use the event's type index via its `type()` function as hash value.

This mechanism enables SMs to forward events to sub-SMs without requiring additional template instantiations just for forwarding as was needed with the `process_any_event` mechanism.
The new mechanism renders the `process_any_event` function obsolete and enables **forwarding of events to sub-SMs in O(1) complexity instead of O(N)**.

Summary:
- Use one dispatch table per state to reduce compiler processing time
  - The algorithms for procesing the STT and states are optimized to go through rows and states only once
  - These dispatch tables are hash tables with type_id as key
- Apply type erasure with boost::any as early as possible and do further processing only with any events
  - each dispatch table only has to cover the events it's handling, no template instantiations required for forwarding events to sub-SMs
- Use `std::any` if C++17 is available (up to 30% runtime impact because of small value optimization in `std::any`)


## Learnings:

- If only a subset needs to be processed, prefer copy_if & transform over fold
- Selecting a template-based function overload in Mp11 seems more efficient than using enable_if/disable_if
