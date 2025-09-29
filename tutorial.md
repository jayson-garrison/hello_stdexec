# std::execution Tutorial

Welcome, the purpose of this article to to provide an overview of the programming model of P2300R10 "std::execution" and showcase some of the basic algorithms. 

I am using NVIDIA's reference implementation `stdexec`.

For convenience, I refer to std::execution / stdexec as `ex`. 

Throughout this article, I often quote [P2300R10](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html) and have appended the user design document in the Appendix. The developer should directly consult this design as it is most explicit and well written. I will summarize in a condensed manner its contents and provide examples. 

# Fundamental Concepts

std::ex introduces three fundamental concepts the developer should know. These are

1. Schedulers
2. Senders
3. Receivers

The developer mainly deals with the first two and implicitly uses receivers. Everything in the execution control paradigm should be thought of in terms of these three concepts.

## Schedulers

> A scheduler is a lightweight handle that represents a strategy for scheduling work onto an execution resource. Since execution resources don’t necessarily manifest in C++ code, it’s not possible to program directly against their API. A scheduler is a solution to that problem: the scheduler concept is defined by a single sender algorithm, schedule, which returns a sender that will complete on an execution resource determined by the scheduler. Logic that you want to run on that context can be placed in the receiver’s completion-signalling method.

Schedulers are the developer's interface to different execution resources. An execution resource is simply something that describes the place of execution and may be used to do work. A primary example of an execution resource is a thread, though there are others kinds of execution resources. 

The scheduler concept is critical to the execution control paradigm because the resource where work is executed and its scheduler are seperated. This allows the developer to declare their work irrespective of where it gets executed.

## Senders

> A sender is an object that describes work. Senders are similar to futures in existing asynchrony designs, but unlike futures, the work that is being done to arrive at the values they will send is also directly described by the sender object itself. A sender is said to send some values if a receiver connected to that sender will eventually receive said values. [...] The way user code is expected to interact with senders is by using sender algorithms. 

The developer describes their work using senders in `std::execution`. A sender, as stated simply above, sends a value or values to a connected receiver. The developer implicitly uses a receiver to connect to their sender, but I will discuss this in the next section. 

When a developer wants to leverage an asynchronous framework to do work they usually want to produce a value--where a value can be a trivial or complex type/data structure. Upon errors or exceptions, the sender can produce an error or if its execution is stopped it may produce a value indicating it was stopped. Therefore, a sender's work completion is described by setting the receivers:

1. Value,
2. Error, or
3. Stopped upon cancellation

The developer produces senders by using the algorithms included in the excecution control library. These algorithms fall into three categories:

1. Sender factories: produce a sender
2. Sender adapters: take a sender, and produce a sender
3. Sender consumers: consume a sender and do not produce a sender

I will explain these three categories in depth later in this article. What the developer should know about these three categories of algorithms, is that they may be composed to create senders. The execution control library provides the `|` operator as the syntax to compose these algorithms and create senders. 

## Receivers

> A receiver is a callback that supports more than one channel. In fact, it supports three of them: `set_value`, `set_error`, and `set_stopped`. [...] Receivers are not a part of the end-user-facing API of this proposal; they are necessary to allow unrelated senders communicate with each other, but the only users who will interact with receivers directly are authors of senders.

So, developers need not worry about directly dealing with receivers, that's one less thing to worry about! All the developer needs to know about receivers is that they are the background mechanism that allows their value(s), error(s), or stopped state produced by senders to be received and used. A receiver is essentuially an object that holds the results of a sender's execution. When a developer executes their sender using a scheduler, the sender is connected to the receiver and the value(s), error(s), or stopped state is received. This will become clear in our examples.

## Putting Schedulers, Senders, and Receivers all Together

In summary, schedulers, senders, and receivers are the three primary concepts that describe the execution control library programming paradigm. In this article, I draw a distinction between a user (developer) and an implementer of `std::execution`. The developer uses schedulers, senders, and receivers (indirectly) to do work on execution resources whereas an implementer develops custom schedulers, senders, algorithms, and receivers directly.

In reality, a developer only needs to use a scheduler and algorithms to leverage the basic features of `stdexecution`, let's consider a common use case.

In C++23 the most straightforward way to manage work using different execution resources is for a developer to create and own something like a `std::jthread` and pass a `std::function` to that thread. In this example, there is no explicit scheduler used by the developer and the function defines the work, not a sender. P2300R10 changes this paradigm. 

If we adopt the `std::execution` paradigm, the developer would use a scheduler as an interface to the execution resource, be it a single thread, a pool of threads, etc., and a sender rather than a `std::function` that describes their work. Here, the sender would begin using the scheduler, which denotes where to do the work, and the sender would be connected to a receiver which obtains the value(s) when the work is completed. 

Let's compare these two approaches side-by-side, first using a `std::jthread`:

```cpp
int result{0};

std::jthread thread{
    [&result](){
        // do some work and get the result:
        result = 42;
    }
};

// do some other work in the main thread

thread.join();
// result is now 42
```

Now, the `std::execution` implementation:

```cpp
// make a thread pool with one thread: this is our execution resource
ex::static_thread_pool tp{1};

// get a handle to the thread pool's scheduler
ex::scheduler auto sched{tp.get_scheduler()};

// define our sender which describes our work:

// begin our pipeline with the schedule algorithm which is a sender factory that returns a
// sender on the given scheduler. this is where our work will be done
ex::sender auto work = ex::schedule(sched) 
// next, use the then algorithm which is a sender adapter that takes a sender (our
// sender on the scheduler defined in the previous step) and appends a function to the
// input sender. then may also return some values.
| ex::then(
    [](){
        // do some work and get the result
        return 42;
    }
);

// do some work in the main thread

// actually start the work now (wait.. this isn't async..?)
auto [result] = ex::sync_wait(std::move(work));
// result is 42

```




# Appendix 

# [P2300R10 Design](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html#design-user) - user side # {#design-user}

## Execution resources describe the place of execution ## {#design-contexts}

An [=execution resource=] is a resource that represents the *place* where
execution will happen. This could be a concrete resource - like a specific
thread pool object, or a GPU - or a more abstract one, like the current thread
of execution. Execution contexts don't need to have a representation in code;
they are simply a term describing certain properties of execution of a function.

## Schedulers represent execution resources ## {#design-schedulers}

A [=scheduler=] is a lightweight handle that represents a strategy for
scheduling work onto an execution resource. Since execution resources don't
necessarily manifest in C++ code, it's not possible to program directly against
their API. A scheduler is a solution to that problem: the scheduler concept is
defined by a single sender algorithm, `schedule`, which returns a sender that
will complete on an execution resource determined by the scheduler. Logic that
you want to run on that context can be placed in the receiver's
completion-signalling method.

<pre highlight="c++">
execution::scheduler auto sch = thread_pool.scheduler();
execution::sender auto snd = execution::schedule(sch);
// snd is a sender (see below) describing the creation of a new execution resource
// on the execution resource associated with sch
</pre>

Note that a particular scheduler type may provide other kinds of scheduling
operations which are supported by its associated execution resource. It is not
limited to scheduling purely using the `execution::schedule` API.

Future papers will propose additional scheduler concepts that extend `scheduler`
to add other capabilities. For example:

* A `time_scheduler` concept that extends `scheduler` to support time-based
    scheduling. Such a concept might provide access to `schedule_after(sched,
    duration)`, `schedule_at(sched, time_point)` and `now(sched)` APIs.
* Concepts that extend `scheduler` to support opening, reading and writing files
    asynchronously.
* Concepts that extend `scheduler` to support connecting, sending data and
    receiving data over the network asynchronously.

## Senders describe work ## {#design-senders}

A [=sender=] is an object that describes work. Senders are similar to futures in
existing asynchrony designs, but unlike futures, the work that is being done to
arrive at the values they will *send* is also directly described by the sender
object itself. A sender is said to *send* some values if a receiver connected
(see [[#design-connect]]) to that sender will eventually *receive* said values.

The primary defining sender algorithm is [[#design-connect]]; this function,
however, is not a user-facing API; it is used to facilitate communication
between senders and various sender algorithms, but end user code is not expected
to invoke it directly.

The way user code is expected to interact with senders is by using [=sender
algorithms=]. This paper proposes an initial set of such sender algorithms,
which are described in [[#design-composable]], [[#design-sender-factories]],
[[#design-sender-adaptors]], and [[#design-sender-consumers]]. For example, here
is how a user can create a new sender on a scheduler, attach a continuation to
it, and then wait for execution of the continuation to complete:

<pre highlight="c++">
execution::scheduler auto sch = thread_pool.scheduler();
execution::sender auto snd = execution::schedule(sch);
execution::sender auto cont = execution::then(snd, []{
    std::fstream file{ "result.txt" };
    file << compute_result;
});

this_thread::sync_wait(cont);
// at this point, cont has completed execution
</pre>

## Senders are composable through sender algorithms ## {#design-composable}

Asynchronous programming often departs from traditional code structure and
control flow that we are familiar with. A successful asynchronous framework must
provide an intuitive story for composition of asynchronous work: expressing
dependencies, passing objects, managing object lifetimes, etc.

The true power and utility of senders is in their composability. With senders,
users can describe generic execution pipelines and graphs, and then run them on
and across a variety of different schedulers. Senders are composed using
[=sender algorithms=]:

* [=sender factories=], algorithms that take no senders and return a sender.
* [=sender adaptors=], algorithms that take (and potentially
    `execution::connect`) senders and return a sender.
* [=sender consumers=], algorithms that take (and potentially
    `execution::connect`) senders and do not return a sender.

## Senders can propagate completion schedulers ## {#design-propagation}

One of the goals of executors is to support a diverse set of execution
resources, including traditional thread pools, task and fiber frameworks (like
\[HPX](https://github.com/STEllAR-GROUP/hpx)
[Legion](https://github.com/StanfordLegion/legion)), and GPUs and other
accelerators (managed by runtimes such as CUDA or SYCL). On many of these
systems, not all execution agents are created equal and not all functions can be
run on all execution agents. Having precise control over the execution resource
used for any given function call being submitted is important on such systems,
and the users of standard execution facilities will expect to be able to express
such requirements.

[[P0443R14]] was not always clear about the <i>place of execution</i> of any
given piece of code. Precise control was present in the two-way execution API
present in earlier executor designs, but it has so far been missing from the
senders design. There has been a proposal ([[P1897R3]]) to provide a number of
sender algorithms that would enforce certain rules on the places of execution of
the work described by a sender, but we have found those sender algorithms to be
insufficient for achieving the best performance on all platforms that are of
interest to us. The implementation strategies that we are aware of result in one
of the following situations:

  1. trying to submit work to one execution resource (such as a CPU thread pool)
      from another execution resource (such as a GPU or a task framework), which
      assumes that all execution agents are as capable as a `std::thread` (which
      they aren't).
  2. forcibly interleaving two adjacent execution graph nodes that are both
      executing on one execution resource (such as a GPU) with glue code that
      runs on another execution resource (such as a CPU), which is prohibitively
      expensive for some execution resources (such as CUDA or SYCL).
  3. having to customise most or all sender algorithms to support an execution
      resource, so that you can avoid problems described in 1. and 2, which we
      believe is impractical and brittle based on months of field experience
      attempting this in [Agency](https://github.com/agency-library/agency).

None of these implementation strategies are acceptable for many classes of
parallel runtimes, such as task frameworks (like
\[HPX](https://github.com/STEllAR-GROUP/hpx)) or accelerator runtimes (like CUDA
or SYCL).

Therefore, in addition to the `starts_on` sender algorithm from [[P1897R3]], we are
proposing a way for senders to advertise what scheduler (and by extension what
execution resource) they will complete on. Any given sender <b>may</b> have
[=completion schedulers=] for some or all of the signals (value, error, or
stopped) it completes with (for more detail on the completion-signals, see
[[#design-receivers]]). When further work is attached to that sender by invoking
sender algorithms, that work will also complete on an appropriate completion
scheduler.

### `execution::get_completion_scheduler` ### {#design-sender-query-get_completion_scheduler}

`get_completion_scheduler` is a query that retrieves the completion scheduler
for a specific completion-signal from a sender's environment. For a sender that
lacks a completion scheduler query for a given signal, calling
`get_completion_scheduler` is ill-formed. If a sender advertises a completion
scheduler for a signal in this way, that sender <b>must</b> ensure that it
[=send|sends=] that signal on an execution agent belonging to an execution
resource represented by a scheduler returned from this function. See
[[#design-propagation]] for more details.

<pre highlight="c++">
execution::scheduler auto cpu_sched = new_thread_scheduler{};
execution::scheduler auto gpu_sched = cuda::scheduler();

execution::sender auto snd0 = execution::schedule(cpu_sched);
execution::scheduler auto completion_sch0 =
  execution::get_completion_scheduler&lt;execution::set_value_t>(get_env(snd0));
// completion_sch0 is equivalent to cpu_sched

execution::sender auto snd1 = execution::then(snd0, []{
    std::cout << "I am running on cpu_sched!\n";
});
execution::scheduler auto completion_sch1 =
  execution::get_completion_scheduler&lt;execution::set_value_t>(get_env(snd1));
// completion_sch1 is equivalent to cpu_sched

execution::sender auto snd2 = execution::continues_on(snd1, gpu_sched);
execution::sender auto snd3 = execution::then(snd2, []{
    std::cout << "I am running on gpu_sched!\n";
});
execution::scheduler auto completion_sch3 =
  execution::get_completion_scheduler&lt;execution::set_value_t>(get_env(snd3));
// completion_sch3 is equivalent to gpu_sched
</pre>

## Execution resource transitions are explicit ## {#design-transitions}

[[P0443R14]] does not contain any mechanisms for performing an execution
resource transition. The only sender algorithm that can create a sender that
will move execution to a *specific* execution resource is `execution::schedule`,
which does not take an input sender. That means that there's no way to construct
sender chains that traverse different execution resources. This is necessary to
fulfill the promise of senders being able to replace two-way executors, which
had this capability.

We propose that, for senders advertising their [=completion scheduler=], all
execution resource transitions <b>must</b> be explicit; running user code
anywhere but where they defined it to run <b>must</b> be considered a bug.

The `execution::continues_on` sender adaptor performs a transition from one
execution resource to another:

<pre highlight="c++">
execution::scheduler auto sch1 = ...;
execution::scheduler auto sch2 = ...;

execution::sender auto snd1 = execution::schedule(sch1);
execution::sender auto then1 = execution::then(snd1, []{
    std::cout << "I am running on sch1!\n";
});

execution::sender auto snd2 = execution::continues_on(then1, sch2);
execution::sender auto then2 = execution::then(snd2, []{
    std::cout << "I am running on sch2!\n";
});

this_thread::sync_wait(then2);
</pre>

## Senders can be either multi-shot or single-shot ## {#design-shot}

Some senders may only support launching their operation a single time, while others may be repeatable
and support being launched multiple times. Executing the operation may consume resources owned by the
sender.

For example, a sender may contain a `std::unique_ptr` that it will be transferring ownership of to the
operation-state returned by a call to `execution::connect` so that the operation has access to
this resource. In such a sender, calling `execution::connect` consumes the sender such that after
the call the input sender is no longer valid. Such a sender will also typically be move-only so that
it can maintain unique ownership of that resource.

A <dfn export=true>single-shot sender</dfn> can only be connected to a receiver
at most once. Its implementation of `execution::connect` only has overloads for
an rvalue-qualified sender. Callers must pass the sender as an rvalue to the
call to `execution::connect`, indicating that the call consumes the sender.

A <dfn export=true>multi-shot sender</dfn> can be connected to multiple
receivers and can be launched multiple times. Multi-shot senders customise
`execution::connect` to accept an lvalue reference to the sender. Callers can
indicate that they want the sender to remain valid after the call to
`execution::connect` by passing an lvalue reference to the sender to call these
overloads. Multi-shot senders should also define overloads of
`execution::connect` that accept rvalue-qualified senders to allow the sender to
be also used in places where only a single-shot sender is required.

If the user of a sender does not require the sender to remain valid after
connecting it to a receiver then it can pass an rvalue-reference to the sender
to the call to `execution::connect`. Such usages should be able to accept either
single-shot or multi-shot senders.

If the caller does wish for the sender to remain valid after the call then it
can pass an lvalue-qualified sender to the call to `execution::connect`. Such
usages will only accept multi-shot senders.

Algorithms that accept senders will typically either decay-copy an input sender
and store it somewhere for later usage (for example as a data-member of the
returned sender) or will immediately call `execution::connect` on the input
sender, such as in `this_thread::sync_wait`.

Some multi-use sender algorithms may require that an input sender be
copy-constructible but will only call `execution::connect` on an rvalue of each
copy, which still results in effectively executing the operation multiple times.
Other multi-use sender algorithms may require that the sender is
move-constructible but will invoke `execution::connect` on an lvalue reference
to the sender.

For a sender to be usable in both multi-use scenarios, it will generally be
required to be both copy-constructible and lvalue-connectable.

## Senders are forkable ## {#design-forkable}

Any non-trivial program will eventually want to fork a chain of senders into
independent streams of work, regardless of whether they are single-shot or
multi-shot. For instance, an incoming event to a middleware system may be
required to trigger events on more than one downstream system. This requires
that we provide well defined mechanisms for making sure that connecting a sender
multiple times is possible and correct.

The `split` sender adaptor facilitates connecting to a sender multiple times,
regardless of whether it is single-shot or multi-shot:

<pre highlight="c++">
auto some_algorithm(execution::sender auto&& input) {
    execution::sender auto multi_shot = split(input);
    // "multi_shot" is guaranteed to be multi-shot,
    // regardless of whether "input" was multi-shot or not

    return when_all(
      then(multi_shot, [] { std::cout << "First continuation\n"; }),
      then(multi_shot, [] { std::cout << "Second continuation\n"; })
    );
}
</pre>

## Senders support cancellation ## {#design-cancellation}

Senders are often used in scenarios where the application may be concurrently
executing multiple strategies for achieving some program goal. When one of these
strategies succeeds (or fails) it may not make sense to continue pursuing the
other strategies as their results are no longer useful.

For example, we may want to try to simultaneously connect to multiple network
servers and use whichever server responds first. Once the first server responds
we no longer need to continue trying to connect to the other servers.

Ideally, in these scenarios, we would somehow be able to request that those
other strategies stop executing promptly so that their resources (e.g. cpu,
memory, I/O bandwidth) can be released and used for other work.

While the design of senders has support for cancelling an operation before it
starts by simply destroying the sender or the operation-state returned from
`execution::connect()` before calling `execution::start()`, there also needs to
be a standard, generic mechanism to ask for an already-started operation to
complete early.

The ability to be able to cancel in-flight operations is fundamental to
supporting some kinds of generic concurrency algorithms.

For example:
* a `when_all(ops...)` algorithm should cancel other operations as soon as one
    operation fails
* a `first_successful(ops...)` algorithm should cancel the other operations as
    soon as one operation completes successfuly
* a generic `timeout(src, duration)` algorithm needs to be able to cancel the
    `src` operation after the timeout duration has elapsed.
* a `stop_when(src, trigger)` algorithm should cancel `src` if `trigger`
    completes first and cancel `trigger` if `src` completes first

The mechanism used for communcating cancellation-requests, or stop-requests,
needs to have a uniform interface so that generic algorithms that compose
sender-based operations, such as the ones listed above, are able to communicate
these cancellation requests to senders that they don't know anything about.

The design is intended to be composable so that cancellation of higher-level
operations can propagate those cancellation requests through intermediate layers
to lower-level operations that need to actually respond to the cancellation
requests.

For example, we can compose the algorithms mentioned above so that child
operations are cancelled when any one of the multiple cancellation conditions
occurs:

<pre highlight="c++">
sender auto composed_cancellation_example(auto query) {
  return stop_when(
    timeout(
      when_all(
        first_successful(
          query_server_a(query),
          query_server_b(query)),
        load_file("some_file.jpg")),
      5s),
    cancelButton.on_click());
}
</pre>

In this example, if we take the operation returned by `query_server_b(query)`,
this operation will receive a stop-request when any of the following happens:

* `first_successful` algorithm will send a stop-request if
    `query_server_a(query)` completes successfully
* `when_all` algorithm will send a stop-request if the
    `load_file("some_file.jpg")` operation completes with an error or stopped
    result.
* `timeout` algorithm will send a stop-request if the operation does not
    complete within 5 seconds.
* `stop_when` algorithm will send a stop-request if the user clicks on the
    "Cancel" button in the user-interface.
* The parent operation consuming the `composed_cancellation_example()` sends a
    stop-request

Note that within this code there is no explicit mention of cancellation,
stop-tokens, callbacks, etc. yet the example fully supports and responds to the
various cancellation sources.

The intent of the design is that the common usage of cancellation in
sender/receiver-based code is primarily through use of concurrency algorithms
that manage the detailed plumbing of cancellation for you. Much like algorithms
that compose senders relieve the user from having to write their own receiver
types, algorithms that introduce concurrency and provide higher-level
cancellation semantics relieve the user from having to deal with low-level
details of cancellation.

### Cancellation design summary ### {#design-cancellation-summary}

The design of cancellation described in this paper is built on top of and
extends the `std::stop_token`-based cancellation facilities added in C++20,
first proposed in [[P2175R0]].

At a high-level, the facilities proposed by this paper for supporting
cancellation include:

* Add a `std::stoppable_token` concept that generalises the interface of the
    `std::stop_token` type to allow other stop token types with different
    implementation strategies.
* Add `std::unstoppable_token` concept for detecting whether a `stoppable_token`
    can never receive a stop-request.
* Add `std::inplace_stop_token`, `std::inplace_stop_source` and
    `std::inplace_stop_callback<CB>` types that provide a more efficient
    implementation of a stop-token for use in structured concurrency situations.
* Add `std::never_stop_token` for use in places where you never want to issue a
    stop-request.
* Add `std::execution::get_stop_token()` CPO for querying the stop-token to use
    for an operation from its receiver's execution environment.
* Add `std::execution::stop_token_of_t<T>` for querying the type of a stop-token
    returned from `get_stop_token()`.

In addition, there are requirements added to some of the algorithms to specify
what their cancellation behaviour is and what the requirements of customisations
of those algorithms are with respect to cancellation.

The key component that enables generic cancellation within sender-based
operations is the `execution::get_stop_token()` CPO. This CPO takes a single
parameter, which is the execution environment of the receiver passed to
`execution::connect`, and returns a `std::stoppable_token` that the operation
can use to check for stop-requests for that operation.

As the caller of `execution::connect` typically has control over the receiver
type it passes, it is able to customise the `std::execution::get_env()` CPO for
that receiver to return an execution environment that hooks the
`execution::get_stop_token()` CPO to return a stop-token that the receiver has
control over and that it can use to communicate a stop-request to the operation
once it has started.

### Support for cancellation is optional ### {#design-cancellation-optional}

Support for cancellation is optional, both on part of the author of the receiver
and on part of the author of the sender.

If the receiver's execution environment does not customise the
`execution::get_stop_token()` CPO then invoking the CPO on that receiver's
environment will invoke the default implementation which returns
`std::never_stop_token`. This is a special `stoppable_token` type that is
statically known to always return `false` from the `stop_possible()` method.

Sender code that tries to use this stop-token will in general result in code
that handles stop-requests being compiled out and having little to no run-time
overhead.

If the sender doesn't call `execution::get_stop_token()`, for example because
the operation does not support cancellation, then it will simply not respond to
stop-requests from the caller.

Note that stop-requests are generally racy in nature as there is often a race
betwen an operation completing naturally and the stop-request being made. If the
operation has already completed or past the point at which it can be cancelled
when the stop-request is sent then the stop-request may just be ignored. An
application will typically need to be able to cope with senders that might
ignore a stop-request anyway.

### Cancellation is inherently racy ### {#design-cancellation-racy}

Usually, an operation will attach a stop callback at some point inside the call
to `execution::start()` so that a subsequent stop-request will interrupt the
logic.

A stop-request can be issued concurrently from another thread. This means the
implementation of `execution::start()` needs to be careful to ensure that, once
a stop callback has been registered, that there are no data-races between a
potentially concurrently-executing stop callback and the rest of the
`execution::start()` implementation.

An implementation of `execution::start()` that supports cancellation will
generally need to perform (at least) two separate steps: launch the operation,
subscribe a stop callback to the receiver's stop-token. Care needs to be taken
depending on the order in which these two steps are performed.

If the stop callback is subscribed first and then the operation is launched,
care needs to be taken to ensure that a stop-request that invokes the
stop callback on another thread after the stop callback is registered but before
the operation finishes launching does not either result in a missed cancellation
request or a data-race. e.g. by performing an atomic write after the launch has
finished executing

If the operation is launched first and then the stop callback is subscribed,
care needs to be taken to ensure that if the launched operation completes
concurrently on another thread that it does not destroy the operation-state
until after the stop callback has been registered. e.g. by having the
`execution::start` implementation write to an atomic variable once it has
finished registering the stop callback and having the concurrent completion
handler check that variable and either call the completion-signalling operation
or store the result and defer calling the receiver's completion-signalling
operation to the `execution::start()` call (which is still executing).

For an example of an implementation strategy for solving these data-races see
[[#example-async-windows-socket-recv]].

### Cancellation design status ### {#design-cancellation-status}

This paper currently includes the design for cancellation as proposed in
[[P2175R0]] - "Composable cancellation for sender-based async operations".
P2175R0 contains more details on the background motivation and prior-art and
design rationale of this design.

It is important to note, however, that initial review of this design in the SG1
concurrency subgroup raised some concerns related to runtime overhead of the
design in single-threaded scenarios and these concerns are still being
investigated.

The design of P2175R0 has been included in this paper for now, despite its
potential to change, as we believe that support for cancellation is a
fundamental requirement for an async model and is required in some form to be
able to talk about the semantics of some of the algorithms proposed in this
paper.

This paper will be updated in the future with any changes that arise from the
investigations into P2175R0.

## Sender factories and adaptors are lazy ## {#design-lazy-algorithms}

In an earlier revision of this paper, some of the proposed algorithms supported
executing their logic eagerly; <i>i.e.</i>, before the returned sender has been
connected to a receiver and started. These algorithms were removed because eager
execution has a number of negative semantic and performance implications.

We have originally included this functionality in the paper because of a
long-standing belief that eager execution is a mandatory feature to be included
in the standard Executors facility for that facility to be acceptable for
accelerator vendors. A particular concern was that we must be able to write
generic algorithms that can run either eagerly or lazily, depending on the kind
of an input sender or scheduler that have been passed into them as arguments. We
considered this a requirement, because the _latency_ of launching work on an
accelerator can sometimes be considerable.

However, in the process of working on this paper and implementations of the
features proposed within, our set of requirements has shifted, as we understood
the different implementation strategies that are available for the feature set
of this paper better, and, after weighing the earlier concerns against the
points presented below, we have arrived at the conclusion that a purely lazy
model is enough for most algorithms, and users who intend to launch work earlier
may write an algorithm to achieve that goal. We have also
come to deeply appreciate the fact that a purely lazy model allows both the
implementation and the compiler to have a much better understanding of what the
complete graph of tasks looks like, allowing them to better optimize the code -
also when targetting accelerators.

### Eager execution leads to detached work or worse ### {#design-lazy-algorithms-detached}

One of the questions that arises with APIs that can potentially return
eagerly-executing senders is "What happens when those senders are destructed
without a call to `execution::connect`?" or similarly, "What happens if a call
to `execution::connect` is made, but the returned operation state is destroyed
before `execution::start` is called on that operation state"?

In these cases, the operation represented by the sender is potentially executing
concurrently in another thread at the time that the destructor of the sender
and/or operation-state is running. In the case that the operation has not
completed executing by the time that the destructor is run we need to decide
what the semantics of the destructor is.

There are three main strategies that can be adopted here, none of which is
particularly satisfactory:

1.  Make this undefined-behaviour - the caller must ensure that any
    eagerly-executing sender is always joined by connecting and starting that
    sender. This approach is generally pretty hostile to programmers,
    particularly in the presence of exceptions, since it complicates the ability
    to compose these operations.

    Eager operations typically need to acquire resources when they are first
    called in order to start the operation early. This makes eager algorithms
    prone to failure. Consider, then, what might happen in an expression such as
    `when_all(eager_op_1(), eager_op_2())`. Imagine `eager_op_1()` starts an
    asynchronous operation successfully, but then `eager_op_2()` throws. For
    lazy senders, that failure happens in the context of the `when_all`
    algorithm, which handles the failure and ensures that async work joins on
    all code paths. In this case though -- the eager case -- the child operation
    has failed even before `when_all` has been called.

    It then becomes the responsibility, not of the algorithm, but of the end
    user to handle the exception and ensure that `eager_op_1()` is joined before
    allowing the exception to propagate. If they fail to do that, they incur
    undefined behavior.

2.  Detach from the computation - let the operation continue in the background -
    like an implicit call to `std::thread::detach()`. While this approach can
    work in some circumstances for some kinds of applications, in general it is
    also pretty user-hostile; it makes it difficult to reason about the safe
    destruction of resources used by these eager operations. In general,
    detached work necessitates some kind of garbage collection; e.g.,
    `std::shared_ptr`, to ensure resources are kept alive until the operations
    complete, and can make clean shutdown nigh impossible.

3.  Block in the destructor until the operation completes. This approach is
    probably the safest to use as it preserves the structured nature of the
    concurrent operations, but also introduces the potential for deadlocking the
    application if the completion of the operation depends on the current thread
    making forward progress.

      The risk of deadlock might occur, for example, if a thread-pool with a
    small number of threads is executing code that creates a sender representing
    an eagerly-executing operation and then calls the destructor of that sender
    without joining it (e.g. because an exception was thrown). If the current
    thread blocks waiting for that eager operation to complete and that eager
    operation cannot complete until some entry enqueued to the thread-pool's
    queue of work is run then the thread may wait for an indefinite amount of
    time. If all threads of the thread-pool are simultaneously performing such
    blocking operations then deadlock can result.

There are also minor variations on each of these choices. For example:

4.  A variation of (1): Call `std::terminate` if an eager sender is destructed
    without joining it. This is the approach that `std::thread` destructor
    takes.

5.  A variation of (2): Request cancellation of the operation before detaching.
    This reduces the chances of operations continuing to run indefinitely in the
    background once they have been detached but does not solve the
    lifetime- or shutdown-related challenges.

6.  A variation of (3): Request cancellation of the operation before blocking on
    its completion. This is the strategy that `std::jthread` uses for its
    destructor. It reduces the risk of deadlock but does not eliminate it.

### Eager senders complicate algorithm implementations ### {#design-lazy-algorithms-complexity}

Algorithms that can assume they are operating on senders with strictly lazy
semantics are able to make certain optimizations that are not available if
senders can be potentially eager. With lazy senders, an algorithm can safely
assume that a call to `execution::start` on an operation state strictly happens
before the execution of that async operation. This frees the algorithm from
needing to resolve potential race conditions. For example, consider an algorithm
`sequence` that puts async operations in sequence by starting an operation only
after the preceding one has completed. In an expression like `sequence(a(),
then(src, [] { b(); }), c())`, one may reasonably assume that `a()`, `b()` and
`c()` are sequenced and therefore do not need synchronisation. Eager algorithms
break that assumption.

When an algorithm needs to deal with potentially eager senders, the potential
race conditions can be resolved one of two ways, neither of which is desirable:

1.  Assume the worst and implement the algorithm defensively, assuming all
    senders are eager. This obviously has overheads both at runtime and in
    algorithm complexity. Resolving race conditions is hard.

2.  Require senders to declare whether they are eager or not with a query.
    Algorithms can then implement two different implementation strategies, one
    for strictly lazy senders and one for potentially eager senders. This
    addresses the performance problem of (1) while compounding the complexity
    problem.

### Eager senders incur cancellation-related overhead ### {#design-lazy-algorithms-runtime}

Another implication of the use of eager operations is with regards to
cancellation. The eagerly executing operation will not have access to the
caller's stop token until the sender is connected to a receiver. If we still
want to be able to cancel the eager operation then it will need to create a new
stop source and pass its associated stop token down to child operations. Then
when the returned sender is eventually connected it will register a stop
callback with the receiver's stop token that will request stop on the eager
sender's stop source.

As the eager operation does not know at the time that it is launched what the
type of the receiver is going to be, and thus whether or not the stop token
returned from `execution::get_stop_token` is an `std::unstoppable_token` or not,
the eager operation is going to need to assume it might be later connected to a
receiver with a stop token that might actually issue a stop request. Thus it
needs to declare space in the operation state for a type-erased stop callback
and incur the runtime overhead of supporting cancellation, even if cancellation
will never be requested by the caller.

The eager operation will also need to do this to support sending a stop request
to the eager operation in the case that the sender representing the eager work
is destroyed before it has been joined (assuming strategy (5) or (6) listed
above is chosen).

### Eager senders cannot access execution resource from the receiver ### {#design-lazy-algorithms-context}

In sender/receiver, contextual information is passed from parent operations to
their children by way of receivers. Information like stop tokens, allocators,
current scheduler, priority, and deadline are propagated to child operations
with custom receivers at the time the operation is connected. That way, each
operation has the contextual information it needs before it is started.

But if the operation is started before it is connected to a receiver, then there
isn't a way for a parent operation to communicate contextual information to its
child operations, which may complete before a receiver is ever attached.

## Schedulers advertise their forward progress guarantees ## {#design-fpg}

To decide whether a scheduler (and its associated execution resource) is
sufficient for a specific task, it may be necessary to know what kind of forward
progress guarantees it provides for the execution agents it creates. The C++
Standard defines the following forward progress guarantees:

* <i>concurrent</i>, which requires that a thread makes progress
    <i>eventually</i>;
* <i>parallel</i>, which requires that a thread makes progress once it executes
    a step; and
* <i>weakly parallel</i>, which does not require that the thread makes progress.

This paper introduces a scheduler query function,
`get_forward_progress_guarantee`, which returns one of the enumerators of a new
`enum` type, `forward_progress_guarantee`. Each enumerator of
`forward_progress_guarantee` corresponds to one of the aforementioned
guarantees.

## Most sender adaptors are pipeable ## {#design-pipeable}

To facilitate an intuitive syntax for composition, most sender adaptors are <dfn
export=true>pipeable</dfn>; they can be composed (<dfn export=true>piped</dfn>)
together with `operator|`. This mechanism is similar to the `operator|`
composition that C++ range adaptors support and draws inspiration from piping in
*nix shells.
Pipeable sender adaptors take a sender as their first parameter and have no
other sender parameters.

`a | b` will pass the sender `a` as the first argument to the pipeable sender
adaptor `b`. Pipeable sender adaptors support partial application of the
parameters after the first. For example, all of the following are equivalent:

<pre highlight="c++">
execution::bulk(snd, N, [] (std::size_t i, auto d) {});
execution::bulk(N, [] (std::size_t i, auto d) {})(snd);
snd | execution::bulk(N, [] (std::size_t i, auto d) {});
</pre>

Piping enables you to compose together senders with a linear syntax. Without it,
you'd have to use either nested function call syntax, which would cause a
syntactic inversion of the direction of control flow, or you'd have to introduce
a temporary variable for each stage of the pipeline. Consider the following
example where we want to execute first on a CPU thread pool, then on a CUDA GPU,
then back on the CPU thread pool:

<table>
<tr>
<th>Syntax Style
<th>Example
<tr>
<th>Function call <br/> (nested)
<td><pre highlight="c++">
auto snd = execution::then(
             execution::continues_on(
               execution::then(
                 execution::continues_on(
                   execution::then(
                     execution::schedule(thread_pool.scheduler())
                     []{ return 123; }),
                   cuda::new_stream_scheduler()),
                 [](int i){ return 123 * 5; }),
               thread_pool.scheduler()),
             [](int i){ return i - 5; });
auto [result] = this_thread::sync_wait(snd).value();
// result == 610
</pre>
<tr>
<th>Function call <br/> (named temporaries)
<td><pre highlight="c++">
auto snd0 = execution::schedule(thread_pool.scheduler());
auto snd1 = execution::then(snd0, []{ return 123; });
auto snd2 = execution::continues_on(snd1, cuda::new_stream_scheduler());
auto snd3 = execution::then(snd2, [](int i){ return 123 * 5; })
auto snd4 = execution::continues_on(snd3, thread_pool.scheduler())
auto snd5 = execution::then(snd4, [](int i){ return i - 5; });
auto [result] = *this_thread::sync_wait(snd4);
// result == 610
</pre>
<tr>
<th>Pipe
<td><pre highlight="c++">
auto snd = execution::schedule(thread_pool.scheduler())
         | execution::then([]{ return 123; })
         | execution::continues_on(cuda::new_stream_scheduler())
         | execution::then([](int i){ return 123 * 5; })
         | execution::continues_on(thread_pool.scheduler())
         | execution::then([](int i){ return i - 5; });
auto [result] = this_thread::sync_wait(snd).value();
// result == 610
</pre>
</table>

Certain sender adaptors are not pipeable, because using the pipeline syntax can
result in confusion of the semantics of the adaptors involved. Specifically, the
following sender adaptors are not pipeable.

* `execution::when_all` and `execution::when_all_with_variant`: Since this
    sender adaptor takes a variadic pack of senders, a partially applied form
    would be ambiguous with a non partially applied form with an arity of one
    less.
* `execution::starts_on`: This sender adaptor changes how the sender passed to it is
    executed, not what happens to its result, but allowing it in a pipeline makes
    it read as if it performed a function more similar to `continues_on`.

Sender consumers could be made pipeable, but we have chosen to not do so.
However, since these are terminal nodes in a pipeline and nothing can be piped
after them, we believe a pipe syntax may be confusing as well as unnecessary, as
consumers cannot be chained. We believe sender consumers read better with
function call syntax.

## A range of senders represents an async sequence of data ## {#design-range-of-senders}

Senders represent a single unit of asynchronous work. In many cases though, what
is being modeled is a sequence of data arriving asynchronously, and you want
computation to happen on demand, when each element arrives. This requires
nothing more than what is in this paper and the range support in C++20. A range
of senders would allow you to model such input as keystrikes, mouse movements,
sensor readings, or network requests.

Given some expression <i>`R`</i> that is a range of senders, consider
the following in a coroutine that returns an async generator type:

    <pre highlight="c++">
    for (auto snd : <i>R</i>) {
      if (auto opt = co_await execution::stopped_as_optional(std::move(snd)))
        co_yield fn(*std::move(opt));
      else
        break;
    }
    </pre>

This transforms each element of the asynchronous sequence <i>`R`</i>
with the function `fn` on demand, as the data arrives. The result is a new
asynchronous sequence of the transformed values.

Now imagine that <i>`R`</i> is the simple expression `views::iota(0)
| views::transform(execution::just)`. This creates a lazy range of senders, each
of which completes immediately with monotonically increasing integers. The above
code churns through the range, generating a new infine asynchronous range of
values [`fn(0)`, `fn(1)`, `fn(2)`, ...].

Far more interesting would be if <i>`R`</i> were a range of senders
representing, say, user actions in a UI. The above code gives a simple way to
respond to user actions on demand.

## Senders can represent partial success ## {#design-partial-success}

Receivers have three ways they can complete: with success, failure, or
cancellation. This begs the question of how they can be used to represent async
operations that *partially* succeed. For example, consider an API that reads
from a socket. The connection could drop after the API has filled in some of the
buffer. In cases like that, it makes sense to want to report both that the
connection dropped and that some data has been successfully read.

Often in the case of partial success, the error condition is not fatal nor does
it mean the API has failed to satisfy its post-conditions. It is merely an extra
piece of information about the nature of the completion. In those cases,
"partial success" is another way of saying "success". As a result, it is
sensible to pass both the error code and the result (if any) through the value
channel, as shown below:

    <pre highlight="c++">
    // Capture a buffer for read_socket_async to fill in
    execution::just(array&lt;byte, 1024>{})
      | execution::let_value([socket](array&lt;byte, 1024>& buff) {
          // read_socket_async completes with two values: an error_code and
          // a count of bytes:
          return read_socket_async(socket, span{buff})
              // For success (partial and full), specify the next action:
            | execution::let_value([](error_code err, size_t bytes_read) {
                if (err != 0) {
                  // OK, partial success. Decide how to deal with the partial results
                } else {
                  // OK, full success here.
                }
              });
        })
    </pre>

In other cases, the partial success is more of a partial *failure*. That happens
when the error condition indicates that in some way the function failed to
satisfy its post-conditions. In those cases, sending the error through the value
channel loses valuable contextual information. It's possible that bundling the
error and the incomplete results into an object and passing it through the error
channel makes more sense. In that way, generic algorithms will not miss the fact
that a post-condition has not been met and react inappropriately.

Another possibility is for an async API to return a *range* of senders: if the
API completes with full success, full error, or cancellation, the returned range
contains just one sender with the result. Otherwise, if the API partially fails
(doesn't satisfy its post-conditions, but some incomplete result is available),
the returned range would have *two* senders: the first containing the partial
result, and the second containing the error. Such an API might be used in a
coroutine as follows:

    <pre highlight="c++">
    // Declare a buffer for read_socket_async to fill in
    array&lt;byte, 1024> buff;

    for (auto snd : read_socket_async(socket, span{buff})) {
      try {
        if (optional&lt;size_t> bytes_read =
              co_await execution::stopped_as_optional(std::move(snd))) {
          // OK, we read some bytes into buff. Process them here....
        } else {
          // The socket read was cancelled and returned no data. React
          // appropriately.
        }
      } catch (...) {
        // read_socket_async failed to meet its post-conditions.
        // Do some cleanup and propagate the error...
      }
    }
    </pre>

Finally, it's possible to combine these two approaches when the API can both
partially succeed (meeting its post-conditions) and partially fail (not meeting
its post-conditions).

## All awaitables are senders ## {#design-awaitables-are-senders}

Since C++20 added coroutines to the standard, we expect that coroutines and
awaitables will be how a great many will choose to express their asynchronous
code. However, in this paper, we are proposing to add a suite of asynchronous
algorithms that accept senders, not awaitables. One might wonder whether and how
these algorithms will be accessible to those who choose coroutines instead of
senders.

In truth there will be no problem because all generally awaitable types
automatically model the `sender` concept. The adaptation is transparent and
happens in the sender customization points, which are aware of awaitables. (By
"generally awaitable" we mean types that don't require custom `await_transform`
trickery from a promise type to make them awaitable.)

For an example, imagine a coroutine type called `task<T>` that knows nothing
about senders. It doesn't implement any of the sender customization points.
Despite that fact, and despite the fact that the `this_thread::sync_wait`
algorithm is constrained with the `sender` concept, the following would compile
and do what the user wants:

```c++
task<int> doSomeAsyncWork();

int main() {
  // OK, awaitable types satisfy the requirements for senders:
  auto o = this_thread::sync_wait(doSomeAsyncWork());
}
```

Since awaitables are senders, writing a sender-based asynchronous algorithm is
trivial if you have a coroutine task type: implement the algorithm as a
coroutine. If you are not bothered by the possibility of allocations and
indirections as a result of using coroutines, then there is no need to ever
write a sender, a receiver, or an operation state.

## Many senders can be trivially made awaitable ## {#design-senders-are-awaitable}

If you choose to implement your sender-based algorithms as coroutines, you'll
run into the issue of how to retrieve results from a passed-in sender. This is
not a problem. If the coroutine type opts in to sender support -- trivial with
the `execution::with_awaitable_senders` utility -- then a large class of senders
are transparently awaitable from within the coroutine.

For example, consider the following trivial implementation of the sender-based
`retry` algorithm:

<pre highlight="c++">
template&lt;class S>
  requires <i>single-sender</i>&lt;S&> // see <a href="#spec-execution.coro_utils.as_awaitable">[exec.as.awaitable]</a>
task&lt;<i>single-sender-value-type</i>&lt;S>> retry(S s) {
  for (;;) {
    try {
      co_return co_await s;
    } catch(...) {
    }
  }
}
</pre>

Only *some* senders can be made awaitable directly because of the fact that
callbacks are more expressive than coroutines. An awaitable expression has a
single type: the result value of the async operation. In contrast, a callback
can accept multiple arguments as the result of an operation. What's more, the
callback can have overloaded function call signatures that take different sets
of arguments. There is no way to automatically map such senders into awaitables.
The `with_awaitable_senders` utility recognizes as awaitables those senders that
send a single value of a single type. To await another kind of sender, a user
would have to first map its value channel into a single value of a single type
-- say, with the `into_variant` sender algorithm -- before `co_await`-ing that
sender.

## Cancellation of a sender can unwind a stack of coroutines ## {#design-native-coro-unwind}

When looking at the sender-based `retry` algorithm in the previous section, we
can see that the value and error cases are correctly handled. But what about
cancellation? What happens to a coroutine that is suspended awaiting a sender
that completes by calling `execution::set_stopped`?

When your task type's promise inherits from `with_awaitable_senders`, what
happens is this: the coroutine behaves as if an *uncatchable exception* had been
thrown from the `co_await` expression. (It is not really an exception, but it's
helpful to think of it that way.) Provided that the promise types of the calling
coroutines also inherit from `with_awaitable_senders`, or more generally
implement a member function called `unhandled_stopped`, the exception unwinds
the chain of coroutines as if an exception were thrown except that it bypasses
`catch(...)` clauses.

In order to "catch" this uncatchable stopped exception, one of the calling
coroutines in the stack would have to await a sender that maps the stopped
channel into either a value or an error. That is achievable with the
`execution::let_stopped`, `execution::upon_stopped`,
`execution::stopped_as_optional`, or `execution::stopped_as_error` sender
adaptors. For instance, we can use `execution::stopped_as_optional` to "catch"
the stopped signal and map it into an empty optional as shown below:

```c++
if (auto opt = co_await execution::stopped_as_optional(some_sender)) {
  // OK, some_sender completed successfully, and opt contains the result.
} else {
  // some_sender completed with a cancellation signal.
}
```

As described in the section <a href="#design-awaitables-are-senders">"All
awaitables are senders"</a>, the sender customization points recognize
awaitables and adapt them transparently to model the sender concept. When
`connect`-ing an awaitable and a receiver, the adaptation layer awaits the
awaitable within a coroutine that implements `unhandled_stopped` in its promise
type. The effect of this is that an "uncatchable" stopped exception propagates
seamlessly out of awaitables, causing `execution::set_stopped` to be called on
the receiver.

Obviously, `unhandled_stopped` is a library extension of the coroutine promise
interface. Many promise types will not implement `unhandled_stopped`. When an
uncatchable stopped exception tries to propagate through such a coroutine, it is
treated as an unhandled exception and `terminate` is called. The solution, as
described above, is to use a sender adaptor to handle the stopped exception
before awaiting it. It goes without saying that any future Standard Library
coroutine types ought to implement `unhandled_stopped`. The author of
[[P1056R1]], which proposes a standard coroutine task type, is in agreement.

## Composition with parallel algorithms ## {#design-parallel-algorithms}

The C++ Standard Library provides a large number of algorithms that offer the
potential for non-sequential execution via the use of execution policies. The
set of algorithms with execution policy overloads are often referred to as
"parallel algorithms", although additional policies are available.

Existing policies, such as `execution::par`, give the implementation permission
to execute the algorithm in parallel. However, the choice of execution resources
used to perform the work is left to the implementation.

We will propose a customization point for combining schedulers with policies in
order to provide control over where work will execute.

<pre highlight="c++">
template&lt;class ExecutionPolicy>
<i>unspecified</i> executing_on(
    execution::scheduler auto scheduler,
    ExecutionPolicy && policy
);
</pre>

This function would return an object of an unspecified type which can be used in
place of an execution policy as the first argument to one of the parallel
algorithms. The overload selected by that object should execute its computation
as requested by `policy` while using `scheduler` to create any work to be run.
The expression may be ill-formed if `scheduler` is not able to support the given
policy.

The existing parallel algorithms are synchronous; all of the effects performed
by the computation are complete before the algorithm returns to its caller. This
remains unchanged with the `executing_on` customization point.

In the future, we expect additional papers will propose asynchronous forms of
the parallel algorithms which (1) return senders rather than values or `void`
and (2) where a customization point pairing a sender with an execution policy
would similarly be used to obtain an object of unspecified type to be provided
as the first argument to the algorithm.

## User-facing sender factories ## {#design-sender-factories}

A [=sender factory=] is an algorithm that takes no senders as parameters and
returns a sender.

### `execution::schedule` ### {#design-sender-factory-schedule}

<pre highlight="c++">
execution::sender auto schedule(
    execution::scheduler auto scheduler
);
</pre>

Returns a sender describing the start of a task graph on the provided scheduler.
See [[#design-schedulers]].

<pre highlight="c++">
execution::scheduler auto sch1 = get_system_thread_pool().scheduler();

execution::sender auto snd1 = execution::schedule(sch1);
// snd1 describes the creation of a new task on the system thread pool
</pre>

### `execution::just` ### {#design-sender-factory-just}

<pre highlight="c++">
execution::sender auto just(
    auto ...&& values
);
</pre>

Returns a sender with no [=completion scheduler|completion schedulers=], which
[=send|sends=] the provided values. The input values are decay-copied into the
returned sender. When the returned sender is connected to a receiver, the values
are moved into the operation state if the sender is an rvalue; otherwise, they
are copied. Then xvalues referencing the values in the operation state are
passed to the receiver's `set_value`.

```c++
execution::sender auto snd1 = execution::just(3.14);
execution::sender auto then1 = execution::then(snd1, [] (double d) {
  std::cout << d << "\n";
});

execution::sender auto snd2 = execution::just(3.14, 42);
execution::sender auto then2 = execution::then(snd2, [] (double d, int i) {
  std::cout << d << ", " << i << "\n";
});

std::vector v3{1, 2, 3, 4, 5};
execution::sender auto snd3 = execution::just(v3);
execution::sender auto then3 = execution::then(snd3, [] (std::vector<int>&& v3copy) {
  for (auto&& e : v3copy) { e *= 2; }
  return std::move(v3copy);
}
auto&& [v3copy] = this_thread::sync_wait(then3).value();
// v3 contains {1, 2, 3, 4, 5}; v3copy will contain {2, 4, 6, 8, 10}.

execution::sender auto snd4 = execution::just(std::vector{1, 2, 3, 4, 5});
execution::sender auto then4 = execution::then(std::move(snd4), [] (std::vector<int>&& v4) {
  for (auto&& e : v4) { e *= 2; }
  return std::move(v4);
});
auto&& [v4] = this_thread::sync_wait(std::move(then4)).value();
// v4 contains {2, 4, 6, 8, 10}. No vectors were copied in this example.
```

### `execution::just_error` ### {#design-sender-factory-just_error}

<pre highlight="c++">
execution::sender auto just_error(
    auto && error
);
</pre>

Returns a sender with no [=completion scheduler|completion schedulers=], which
completes with the specified error. If the provided error is an lvalue
reference, a copy is made inside the returned sender and a non-const lvalue
reference to the copy is sent to the receiver's `set_error`. If the provided
value is an rvalue reference, it is moved into the returned sender and an rvalue
reference to it is sent to the receiver's `set_error`.

### `execution::just_stopped` ### {#design-sender-factory-just_stopped}

<pre highlight="c++">
execution::sender auto just_stopped();
</pre>

Returns a sender with no [=completion scheduler|completion schedulers=], which
completes immediately by calling the receiver's `set_stopped`.

### `execution::read_env` ### {#design-sender-factory-read}

<pre highlight="c++">
execution::sender auto read_env(auto tag);
</pre>

Returns a sender that reaches into a receiver's environment and pulls out the
current value associated with the customization point denoted by `Tag`. It then
sends the value read back to the receiver through the value channel. For
instance, `read_env(get_scheduler)` is a sender that asks the
receiver for the currently suggested `scheduler` and passes it to the receiver's
`set_value` completion-signal.

This can be useful when scheduling nested dependent work. The following sender
pulls the current schduler into the value channel and then schedules more work
onto it.

    <pre highlight="c++">
    execution::sender auto task =
      execution::read_env(get_scheduler)
        | execution::let_value([](auto sched) {
            return execution::starts_on(sched, <i>some nested work here</i>);
        });

    this_thread::sync_wait( std::move(task) ); // wait for it to finish
    </pre>

This code uses the fact that `sync_wait` associates a scheduler with the
receiver that it connects with `task`. `read_env(get_scheduler)` reads that scheduler
out of the receiver, and passes it to `let_value`'s receiver's `set_value`
function, which in turn passes it to the lambda. That lambda returns a new
sender that uses the scheduler to schedule some nested work onto `sync_wait`'s
scheduler.

## User-facing sender adaptors ## {#design-sender-adaptors}

A [=sender adaptor=] is an algorithm that takes one or more senders, which it
may `execution::connect`, as parameters, and returns a sender, whose completion
is related to the sender arguments it has received.

Sender adaptors are <i>lazy</i>, that is, they are never allowed to submit any
work for execution prior to the returned sender being [=started=] later on, and
are also guaranteed to not start any input senders passed into them. Sender
consumers such as [[#design-sender-consumer-sync_wait]] start senders.

For more implementer-centric description of starting senders, see
[[#design-laziness]].

### `execution::continues_on` ### {#design-sender-adaptor-continues_on}

<pre highlight="c++">
execution::sender auto continues_on(
    execution::sender auto input,
    execution::scheduler auto scheduler
);
</pre>

Returns a sender describing the transition from the execution agent of the input
sender to the execution agent of the target scheduler. See
[[#design-transitions]].

<pre highlight="c++">
execution::scheduler auto cpu_sched = get_system_thread_pool().scheduler();
execution::scheduler auto gpu_sched = cuda::scheduler();

execution::sender auto cpu_task = execution::schedule(cpu_sched);
// cpu_task describes the creation of a new task on the system thread pool

execution::sender auto gpu_task = execution::continues_on(cpu_task, gpu_sched);
// gpu_task describes the transition of the task graph described by cpu_task to the gpu
</pre>

### `execution::then` ### {#design-sender-adaptor-then}

<pre highlight="c++">
execution::sender auto then(
    execution::sender auto input,
    std::invocable<<i>values-sent-by(input)</i>...> function
);
</pre>

`then` returns a sender describing the task graph described by the input sender,
with an added node of invoking the provided function with the values
[=send|sent=] by the input sender as arguments.

`then` is **guaranteed** to not begin executing `function` until the returned
sender is started.

<pre highlight="c++">
execution::sender auto input = get_input();
execution::sender auto snd = execution::then(input, [](auto... args) {
    std::print(args...);
});
// snd describes the work described by pred
// followed by printing all of the values sent by pred
</pre>

This adaptor is included as it is necessary for writing any sender code that
actually performs a useful function.

### `execution::upon_*` ### {#design-sender-adaptor-upon}

<pre highlight="c++">
execution::sender auto upon_error(
    execution::sender auto input,
    std::invocable&lt;<i>errors-sent-by(input)</i>...> function
);

execution::sender auto upon_stopped(
    execution::sender auto input,
    std::invocable auto function
);
</pre>

`upon_error` and `upon_stopped` are similar to `then`, but where `then` works
with values sent by the input sender, `upon_error` works with errors, and
`upon_stopped` is invoked when the "stopped" signal is sent.

### `execution::let_*` ### {#design-sender-adaptor-let}

<pre highlight="c++">
execution::sender auto let_value(
    execution::sender auto input,
    std::invocable<<i>values-sent-by(input)</i>...> function
);

execution::sender auto let_error(
    execution::sender auto input,
    std::invocable<<i>errors-sent-by(input)</i>...> function
);

execution::sender auto let_stopped(
    execution::sender auto input,
    std::invocable auto function
);
</pre>

`let_value` is very similar to `then`: when it is started, it invokes the
provided function with the values [=send|sent=] by the input sender as
arguments. However, where the sender returned from `then` sends exactly what
that function ends up returning -
`let_value` requires that the function return a sender, and the sender returned
by `let_value` sends the values sent by the sender returned from the callback.
This is similar to the notion of "future unwrapping" in future/promise-based
frameworks.

`let_value` is **guaranteed** to not begin executing `function` until the
returned sender is started.

`let_error` and `let_stopped` are similar to `let_value`, but where `let_value`
works with values sent by the input sender, `let_error` works with errors, and
`let_stopped` is invoked when the "stopped" signal is sent.

### `execution::starts_on` ### {#design-sender-adaptor-starts_on}

<pre highlight="c++">
execution::sender auto starts_on(
    execution::scheduler auto sched,
    execution::sender auto snd
);
</pre>

Returns a sender which, when started, will start the provided sender on an
execution agent belonging to the execution resource associated with the provided
scheduler. This returned sender has no [=completion scheduler|completion
schedulers=].

### `execution::into_variant` ### {#design-sender-adaptor-into_variant}

<pre highlight="c++">
execution::sender auto into_variant(
    execution::sender auto snd
);
</pre>

Returns a sender which sends a variant of tuples of all the possible sets of
types sent by the input sender. Senders can send multiple sets of values
depending on runtime conditions; this is a helper function that turns them into
a single variant value.

### `execution::stopped_as_optional` ### {#design-sender-adaptor-stopped_as_optional}

<pre highlight="c++">
execution::sender auto stopped_as_optional(
    <i>single-sender</i> auto snd
);
</pre>

Returns a sender that maps the value channel from a `T` to an
`optional<decay_t<T>>`, and maps the stopped channel to a value of an empty
`optional<decay_t<T>>`.

### `execution::stopped_as_error` ### {#design-sender-adaptor-stopped_as_error}

<pre highlight="c++">
template&lt;move_constructible Error>
execution::sender auto stopped_as_error(
    execution::sender auto snd,
    Error err
);
</pre>

Returns a sender that maps the stopped channel to an error of `err`.

### `execution::bulk` ### {#design-sender-adaptor-bulk}

<pre highlight="c++">
execution::sender auto bulk(
    execution::sender auto input,
    std::integral auto shape,
    invocable&lt;decltype(size), <i>values-sent-by(input)</i>...> function
);
</pre>

Returns a sender describing the task of invoking the provided function with
every index in the provided shape along with the values sent by the input
sender. The returned sender completes once all invocations have completed, or an
error has occurred. If it completes by sending values, they are equivalent to
those sent by the input sender.

No instance of `function` will begin executing until the returned sender is
started. Each invocation of `function` runs in an execution agent whose forward
progress guarantees are determined by the scheduler on which they are run. All
agents created by a single use of `bulk` execute with the same guarantee. The
number of execution agents used by `bulk` is not specified. This allows a
scheduler to execute some invocations of the `function` in parallel.

In this proposal, only integral types are used to specify the shape of the bulk
section. We expect that future papers may wish to explore extensions of the
interface to explore additional kinds of shapes, such as multi-dimensional
grids, that are commonly used for parallel computing tasks.

### `execution::split` ### {#design-sender-adaptor-split}

<pre highlight="c++">
execution::sender auto split(execution::sender auto sender);
</pre>

If the provided sender is a multi-shot sender, returns that sender. Otherwise,
returns a multi-shot sender which sends values equivalent to the values sent by
the provided sender. See [[#design-shot]].

### `execution::when_all` ### {#design-sender-adaptor-when_all}

<pre highlight="c++">
execution::sender auto when_all(
    execution::sender auto ...inputs
);

execution::sender auto when_all_with_variant(
    execution::sender auto ...inputs
);
</pre>

`when_all` returns a sender that completes once all of the input senders have
completed. It is constrained to only accept senders that can complete with a
single set of values (_i.e._, it only calls one overload of `set_value` on its
receiver). The values sent by this sender are the values sent by each of the
input senders, in order of the arguments passed to `when_all`. It completes
inline on the execution resource on which the last input sender completes,
unless stop is requested before `when_all` is started, in which case it
completes inline within the call to `start`.

`when_all_with_variant` does the same, but it adapts all the input senders using
`into_variant`, and so it does not constrain the input arguments as `when_all`
does.

The returned sender has no [=completion scheduler|completion schedulers=].

<pre highlight="c++">
execution::scheduler auto sched = thread_pool.scheduler();

execution::sender auto sends_1 = ...;
execution::sender auto sends_abc = ...;

execution::sender auto both = execution::when_all(
    sends_1,
    sends_abc
);

execution::sender auto final = execution::then(both, [](auto... args){
    std::cout << std::format("the two args: {}, {}", args...);
});
// when final executes, it will print "the two args: 1, abc"
</pre>

## User-facing sender consumers ## {#design-sender-consumers}

A [=sender consumer=] is an algorithm that takes one or more senders, which it
may `execution::connect`, as parameters, and does not return a sender.

### `this_thread::sync_wait` ### {#design-sender-consumer-sync_wait}

<pre highlight="c++">
auto sync_wait(
    execution::sender auto sender
) requires (<i>always-sends-same-values</i>(sender))
    -> std::optional&lt;std::tuple&lt;<i>values-sent-by</i>(sender)>>;
</pre>

`this_thread::sync_wait` is a sender consumer that submits the work described by
the provided sender for execution,
blocking <b>the current `std::thread` or thread of `main`</b> until the work is
completed, and returns an optional tuple of values that were sent by the
provided sender on its completion of work. Where
[[#design-sender-factory-schedule]] and [[#design-sender-factory-just]] are
meant to <i>enter</i> the domain of senders, `sync_wait` is one way to <i>exit</i>
the domain of senders, retrieving the result of the task graph.

If the provided sender sends an error instead of values, `sync_wait` throws that
error as an exception, or rethrows the original exception if the error is of
type `std::exception_ptr`.

If the provided sender sends the "stopped" signal instead of values, `sync_wait`
returns an empty optional.

For an explanation of the `requires` clause, see [[#design-typed]]. That clause
also explains another sender consumer, built on top of `sync_wait`:
`sync_wait_with_variant`.

Note: This function is specified inside `std::this_thread`, and not inside
`execution`. This is because `sync_wait` has to block the <i>current</i>
execution agent, but determining what the current execution agent is is not
reliable. Since the standard does not specify any functions on the current
execution agent other than those in `std::this_thread`, this is the flavor of
this function that is being proposed. If C++ ever obtains fibers, for instance,
we expect that a variant of this function called `std::this_fiber::sync_wait`
would be provided. We also expect that runtimes with execution agents that use
different synchronization mechanisms than `std::thread`'s will provide their own
flavors of `sync_wait` as well (assuming their execution agents have the means
to block in a non-deadlock manner).