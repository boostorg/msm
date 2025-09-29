# Boost MSM backmp11 backend

This README file is temporary and contains information about `backmp11`, a new backend that is mostly backwards-compatible with `back`. It is currently in **experimental** stage, thus some details about the compatibility might change (feedback welcome!). This file's contents should eventually move into the MSM documentation.

The new backend has the following goals:

- reduce compilation runtime and RAM usage
- reduce state machine runtime

It is named after the metaprogramming library Boost Mp11, the main contributor to the optimizations. Usages of MPL are replaced with Mp11 to get rid of the costly C++03 emulation of variadic templates.

## Behavioral changes


## New features

### Universal visitor API

The need to define a BaseState, accept_sig and accept method in the frontend is removed.

Instead there is a universal visitor API that supports two overloads via tag dispatch to either iterate over only active states or all states:
- void state_machine::visit(F&& f) // Same as with active_states_t
- void state_machine::visit(F&& f, back::active_states_t)
- void state_machine::visit(F&& f, back::all_states_t)

The functor f needs to fulfill the signature `void (auto& state)`.

Also these bugs are fixed:
- If the SM is not started yet, no active state is visited instead of the initial state(s)
- If the SM is stopped, no active state is visited instead of the last active state(s)


## Resolved limitations


## Breaking changes

### The targeted minimum C++ version is C++17

C++11 brings the strongly needed variadic template support for MSM, but later C++ versions provide other important features - for example C++17's `if constexpr`.


### The eUML frontend is not supported

The support of EUML induces longer compilation times by the need to include the Boost proto headers and applying C++03 variadic template emulation. If you want to use a UML-like syntax, please try out the new PUML frontend.


### The backend's constructor does not allow initialization of states and `set_states` is not available

There were some caveats with one constructor that was used for different use cases: On the one hand some arguments were immediately forwarded to the frontend's constructor, on the other hand the stream operator was used to identify other arguments in the constructor as states, to copy them into the state machine. Besides the syntax of the later being rather unusual, when doing both at once the syntax becomes too difficult to understand; even more so if states within hierarchical sub state machines were initialized in this fashion.

In order to keep API of the constructor simpler and less ambiguous, it only supports forwarding arguments to the frontend and no more.
Also the `set_states` API is removed. If setting a state is required, this can still be done (in a little more verbose, but also more direct & explicit fashion) by getting a reference to the desired state via `get_state` and then assigning the desired new state to it.


### `sm_ptr` support is removed

Not needed with the functor frontend and was already deprecated in MSM, thus removed in MSM2.


### The method `visit_current_states` is removed

Please use the universal visitor API instead.


### The method `get_state_by_id` is removed

If you really need to get a state by id, please use the universal visitor API to implement the function on your own.
The backmp11 state_machine has a new method to support getting the id of a state in the visitor:

```
template<typename State>
static constexpr int get_state_id(const State&);
```

### The pointer overload of `get_state` is removed

Similarly to the STL's `std::get` of a tuple, the only sensible template parameter for `get_state` is `T` returning a `T&`.
The overload for a `T*` is removed and the `T&` is discouraged, although still supported.
If you need to get a state by its address, use the address operator after you have received the state,


## How to use it

The backend and both its policies `favor_runtime_speed` and `favor_compile_time` should be compatible with existing code. Required replacements to try it out:
- use `boost::msm::backmp11::state_machine` in place of `boost::msm::back::state_machine` and
- use `boost::msm::backmp11::favor_compile_time` in place of `boost::msm::back::favor_compile_time`

When using the `favor_compile_time` policy, a different macro to generate missing parts of a SM is needed:
- use `BOOST_MSM_BACKMP11_GENERATE_DISPATCH_TABLE(<fsmname>)` in place of `BOOST_MSM_BACK_GENERATE_PROCESS_EVENT(<fsmname>)`


## Applied optimizations

- Replacement of CPU-intensive calls (due to C++03 recursion from MPL) with Mp11
- Applied type punning where useful (to reduce template instantiations, e.g. std::deque & other things around the dispatch table)


### `favor_runtime_speed` policy

Summary:
- Optimized cell initialization with initializer arrays (to reduce template instantiations)
- Default-initialized everything and afterwards only the defer transition cells


### `favor_compile_time` policy

Once an event is given to the FSM for processing, it is immediately converted to `any` and processing continues with this `any` event.
The structure of the dispatch table has been reworked, one dispatch table is created per state as a hash map.
The state dispatch tables are designed to directly work with the `any` event, they use the event's type index via its `type()` function as hash value.

This mechanism enables SMs to forward events to sub-SMs without requiring additional template instantiations just for forwarding as was needed with the `process_any_event` mechanism.
The new mechanism renders the `process_any_event` function obsolete and enables **forwarding of events to sub-SMs in O(1) complexity instead of O(N)**.

Summary:
- Use one dispatch table per state to reduce compiler processing time
  - The algorithms for processing the STT and states are optimized to go through rows and states only once
  - These dispatch tables are hash tables with type_id as key
- Apply type erasure with `any` as early as possible and do further processing only with any events
  - each dispatch table only has to cover the events it's handling, no template instantiations required for forwarding events to sub-SMs


### Learnings:

- If only a subset needs to be processed, prefer copy_if & transform over fold
- Selecting a template-based function overload in Mp11 seems more efficient than using enable_if/disable_if
