# Boost MSM backmp11 back-end

This README file is temporary and contains information about `backmp11`, a new back-end that is mostly backwards-compatible with `back`. It is currently in **experimental** stage, thus some details about the compatibility might change (feedback welcome!). This file's contents should eventually move into the MSM documentation.

The new back-end has the following goals:

- reduce compilation runtime and RAM usage
- reduce state machine runtime
- provide new features

It is named after the metaprogramming library Boost Mp11, the main contributor to the optimizations. Usages of MPL are replaced with Mp11 to get rid of the costly C++03 emulation of variadic templates.


## New features

### Universal visitor API

The need to define a BaseState, accept_sig and accept method in the frontend is removed.

Instead there is a universal visitor API that supports two overloads via tag dispatch to either iterate over only active states or all states:

```cpp
template<typename Visitor>
void state_machine::visit(Visitor&& visitor); // Same as with active_states_t
template<typename Visitor>
void state_machine::visit(Visitor&& visitor, backmp11::active_states_t);
template<typename Visitor>
void state_machine::visit(Visitor&& visitor, backmp11::all_states_t);
```

The visitor needs to fulfill the signature requirement for all sub-states present in the state machine:

```cpp
template<typename State>
void operator()(State& state);
```

Also these bugs are fixed:
- If the SM is not started yet, no active state is visited instead of the initial state(s)
- If the SM is stopped, no active state is visited instead of the last active state(s)


### Method to check whether a state is active

A new method `is_state_active` can be used to check whether a state is currently active:

```cpp
template <typename State>
bool state_machine::is_state_active() const;
```

If the type of the state appears multiple times in a hierarchical state machine, the method returns true if any of the states are active.


### Method to reset the state machine

A new method `reset()` can be used to reset the state machine back to its initial state after construction.

```cpp
void state_machine::reset();
```

The behaviors are start and stop are:
- if `start()` is called for a running state machine, the call is ignored
- if `stop()` is called on a stopped (not running) state machine, the call is ignored


### Simplified state machine signature

The signature has been simplified to facilitate sharing configurations between state machines. The new signature looks as follows:

```cpp
template <
    class TFrontEnd,
    class TConfig = default_state_machine_config,
    class TDerived = void
>
class state_machine;
```

The configuration of the state machine can be defined with a config structure. The default config looks as follows:

```cpp
struct default_state_machine_config
{
    using compile_policy = favor_runtime_speed;
    using context = no_context;
    using root_sm = no_root_sm;
    using fsm_parameter = processing_sm;
    template<typename T>
    using queue_container = std::deque<T>;
};

using state_machine_config = default_state_machine_config;

...

// User-defined config
struct CustomStateMachineConfig : public state_machine_config
{
    using compile_policy = favor_compile_time;
};
```

**IMPORTANT:** The parameter `TDerived` has to be set to the derived class, if the `state_machine` class gets extended.


## New state machine config for defining a context

The config `context` sets up a context member in the state machine for dependency injection.

If `using context = Context;` is defined in the config, a reference to it has to be passed to the state machine constructor as first argument.
The following API becomes available to access it from within the state machine:

```cpp
Context& state_machine::get_context();
const Context& state_machine::get_context() const;
```


## New state machine config for defining a root sm

The config `root_sm` defines the type of the root state machine of hierarchical state machines. The root sm depicts the uppermost state machine.

If `using root_sm = RootSm;` is defined in the config, the following API becomes available to access it from within any sub-state machine:

```cpp
RootSm& state_machine::get_root_sm();
const RootSm& state_machine::get_root_sm() const;
```

It is strongly recommended to configure the `root_sm` in hierarchical state machines, even if access to it is not required.
It reduces the compilation time, because it enables the back-end to instantiate the full set of construction-related methods
only for the root and it can omit them for sub-state machines.

**IMPORTANT:** If a `context` is used in a hierarchical state machine, then also the `root_sm` must be set.
The `context` is then automatically accessed through the `root_sm`.


## New state machine config for defining the `Fsm` parameter of actions and guards

The config `fsm_parameter` defines the instance of the `Fsm& fsm` parameter that is passed to actions and guards in hierarchical state machines.
It can be either the `processing_fsm` (the `state_machine` processing the event, default) or the `root_sm`.

If `using processing_fsm = root_sm;` is defined in the config, the setting takes effect.

**IMPORTANT:** If the `processing_fsm` is set to `root_sm`, then also the `root_sm` must be set.


## Generic support for serializers

The `state_machine` allows access to its private members for serialization purposes with a friend:

```cpp
// Class boost::msm::backmp11::state_machine
template<typename T, typename A0, typename A1, typename A2>
friend void serialize(T&, state_machine<A0, A1, A2>&);
```

A similar friend declaration is available in the `history_impl` classes.


## Changes with respect to `back`

### The required minimum C++ version is C++17

C++11 brings the strongly needed variadic template support for MSM, but later C++ versions provide other important features - for example C++17's `if constexpr`.


### The signature of the state machine is changed

Please refer to the simplified state machine signature above for more information.


### The history policy of a state machine is defined in the front-end instead of the back-end

The definition of the history policy is closer related to the front-end, and defining it there ensures that state machine configs can be shared between back-ends.
The definition looks as follows:

```cpp
struct no_history {};

template <typename... Events>
struct shallow_history {};

struct always_shallow_history {};

...

// User-defined state machine
struct Playing_ : public msm::front::state_machine_def<Playing_>
{
    using history = msm::front::shallow_history<end_pause>;
    ...
};
```


### The public API of `state_machine` is refactored

All methods that should not be part of the public API are removed from it, redundant methods are removed as well. A few other methods have been renamed.
The following adapter pseudo-code showcases the differences to the `back` API:

```cpp
class state_machine_adapter
{
    // The new API returns a const std::array<...>&.
    const int* current_state() const
    {
        return &this->get_active_state_ids()[0];
    }

    // The history can be accessed like this,
    // but it has to be configured in the front-end.
    auto& get_history()
    {
        return this->m_history;
    }

    auto& get_message_queue()
    {
        return this->get_events_queue();
    }

    size_t get_message_queue_size() const
    {
        return this->get_events_queue().size();
    }

    void execute_queued_events()
    {
        this->process_queued_events();
    }

    void execute_single_queued_event()
    {
        this->process_single_queued_event();
    }

    auto& get_deferred_queue()
    {
        return this->get_deferred_events_queue();
    }

    void clear_deferred_queue()
    {
        this->get_deferred_events_queue().clear();
    }

    // No adapter.
    // Superseded by the visitor API.
    // void visit_current_states(...) {...}

    // No adapter.
    // States can be set with `get_state<...>()` or the visitor API.
    // void set_states(...) {...}

    // No adapter.
    // Could be implemented with the visitor API.
    // auto get_state_by_id(int id) {...}
};
```

A working code example of such an adapter is available in [the tests](../../../../test/Backmp11.hpp).
It can be copied and adapted if needed, though this class is internal to the tests and not planned to be supported officially.

Further details about the applied API changes:

#### Support for `boost::serialization` is removed

The back-end aims to support serialization in general, but without providing a concrete implementation for a specific serialization library.
If you want to use `boost::serialization` for your state machine, you can look into the [state machine adapter](../../../../test/Backmp11.hpp) from the tests for an example how to set it up.


#### The back-end's constructor does not allow initialization of states and `set_states` is removed

There were some caveats with one constructor that was used for different use cases: On the one hand some arguments were immediately forwarded to the frontend's constructor, on the other hand the stream operator was used to identify other arguments in the constructor as states, to copy them into the state machine. Besides the syntax of the later being rather unusual, when doing both at once the syntax becomes too difficult to understand; even more so if states within hierarchical sub state machines were initialized in this fashion.

In order to keep the API of the constructor simpler and less ambiguous, it only supports forwarding arguments to the frontend and no more.
Also the `set_states` API is removed. If setting a state is required, this can still be done (in a little more verbose, but also more direct & explicit fashion) by getting a reference to the desired state via `get_state` and then assigning the desired new state to it.


#### The method `get_state_by_id` is removed

If you really need to get a state by id, please use the universal visitor API to implement the function on your own.
The backmp11 state_machine has a new method to support getting the id of a state in the visitor:

```cpp
template<typename State>
static constexpr int state_machine::get_state_id(const State&);
```


#### The pointer overload of `get_state` is removed

Similarly to the STL's `std::get` of a tuple, the only sensible template parameter for `get_state` is `T` returning a `T&`.
The overload for a `T*` is removed and the `T&` is discouraged, although still supported.
If you need to get a state by its address, use the address operator after you have received the state.


### `boost::any` as Kleene event is replaced by `std::any`

To reduce the amount of necessary header inclusions `backmp11` uses `std::any` for defining Kleene events instead of `boost::any`.
You can still opt in to use `boost::any` by explicitly including `boost/msm/event_traits.h`.


### The eUML frontend support is removed

The support of EUML induces longer compilation times by the need to include the Boost proto headers and applying C++03 variadic template emulation. If you want to use a UML-like syntax, please try out the new PUML frontend.


### The fsm check and find region support is removed

The implementation of these two features depends on mpl_graph, which induces high compilation times.


### `sm_ptr` support is removed

Not needed with the functor frontend and was already deprecated, thus removed in `backmp11`.


## How to use it

The back-end with both its compile policies `favor_runtime_speed` and `favor_compile_time` should be mostly compatible with existing code.

Required replacements to try it out:
- for the state machine use `boost::msm::backmp11::state_machine` in place of `boost::msm::back::state_machine` and
- for configuring the compile policy and more use `boost::msm::backmp11::state_machine_config`
- if you encounter API-incompatibilities please check the [details above](#changes-with-respect-to-back) for reference

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
