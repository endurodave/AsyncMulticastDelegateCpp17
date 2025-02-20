# Asynchronous Multicast Delegates in C++17

A C++ standards compliant delegate library capable of targeting any callable function synchronously or asynchronously.

## ⚠️ Deprecated Repository

> **Warning:** This repository is no longer maintained. Please use the modern delegate library in link below.

## New Repository 

[DelegateMQ](https://github.com/endurodave/DelegateMQ) - Invoke any C++ callable function synchronously, asynchronously, or on a remote endpoint.

# Table of Contents

- [Asynchronous Multicast Delegates in C++17](#asynchronous-multicast-delegates-in-c17)
  - [⚠️ Deprecated Repository](#️-deprecated-repository)
  - [New Repository](#new-repository)
- [Table of Contents](#table-of-contents)
- [Preface](#preface)
  - [Related Repositories](#related-repositories)
  - [Library Comparison](#library-comparison)
- [Introduction](#introduction)
- [Delegates Background](#delegates-background)
- [Quick Start](#quick-start)
  - [Publisher](#publisher)
  - [Subscriber](#subscriber)
- [Project Build](#project-build)
  - [Windows Visual Studio](#windows-visual-studio)
  - [Linux Make](#linux-make)
- [Using the Code](#using-the-code)
  - [Synchronous Delegates](#synchronous-delegates)
  - [Asynchronous Non-Blocking Delegates](#asynchronous-non-blocking-delegates)
  - [Bind to std::shared\_ptr](#bind-to-stdshared_ptr)
  - [Caution Using Raw Object Pointers](#caution-using-raw-object-pointers)
  - [Asynchronous Blocking Delegates](#asynchronous-blocking-delegates)
  - [Asynchronous Lambda Invocation](#asynchronous-lambda-invocation)
- [Delegate Library](#delegate-library)
  - [Worker Thread (std::thread)](#worker-thread-stdthread)
- [Delegate Containers](#delegate-containers)
- [Examples](#examples)
  - [SysData Example](#sysdata-example)
  - [SysDataClient Example](#sysdataclient-example)
  - [SysDataNoLock Example](#sysdatanolock-example)
  - [SysDataNoLock Reinvoke Example](#sysdatanolock-reinvoke-example)
  - [SysDataNoLock Blocking Reinvoke Example](#sysdatanolock-blocking-reinvoke-example)
  - [Timer Example](#timer-example)
- [Summary](#summary)
- [Which Callback Implementation?](#which-callback-implementation)
  - [Asynchronous Multicast Callbacks in C](#asynchronous-multicast-callbacks-in-c)
  - [Asynchronous Multicast Callbacks with Inter-Thread Messaging](#asynchronous-multicast-callbacks-with-inter-thread-messaging)
  - [Asynchronous Multicast Delegates in C++](#asynchronous-multicast-delegates-in-c)
  - [Asynchronous Multicast Delegates in Modern C++](#asynchronous-multicast-delegates-in-modern-c)
- [Limitations](#limitations)
- [References](#references)
- [Conclusion](#conclusion)


# Preface

## Related Repositories

* <a href="https://github.com/endurodave/AsyncStateMachine">Asynchronous State Machine Design in C++</a> - an asynchronous C++ state machine implemented using an asynchronous delegate library.
* <a href="https://github.com/endurodave/IntegrationTestFramework">Integration Test Framework using Google Test and Delegates</a> - a multi-threaded C++ software integration test framework using Google Test and Delegate libraries.

## Library Comparison

<p>Asynchronous function invocation allows for easy movement of data between threads. The table below summarizes the various asynchronous function invocation implementations available in C and C++.</p>

| Repository                                                                                            | Language | Key Delegate Features                                                                                                                                                                                                               | Notes                                                                                                                                                                                                      |
|-------------------------------------------------------------------------------------------------------|----------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <a href="https://github.com/endurodave/AsyncMulticastDelegateModern">AsyncMulticastDelegateModern</a> | C++17    | * Function-like template syntax<br> * Any delegate target function type (member, static, free, lambda)<br>  * N target function arguments<br> * N delegate subscribers<br> * Variadic templates<br> * Template metaprogramming      | * Most generic implementation<br> * Lowest lines of source code<br> * Slowest of all implementations<br> * Optional fixed block allocator support<br> * No remote delegate support<br> * Complex metaprogramming |
| <a href="https://github.com/endurodave/AsyncMulticastDelegateCpp17">AsyncMulticastDelegateCpp17</a>   | C++17    | * Function-like template syntax<br> * Any delegate target function type (member, static, free, lambda)<br> * 5 target function arguments<br> * N delegate subscribers<br> * Optional fixed block allocator<br> * Variadic templates | * Selective compile using constexpr<br> * Avoids complex metaprogramming<br> * Faster than AsyncMulticastDelegateModern<br> * No remote delegate support                                                   |
| <a href="https://github.com/endurodave/AsyncMulticastDelegateCpp11">AsyncMulticastDelegateCpp11</a>   | C++11    | * Function-like template syntax<br> * Any delegate target function type (member, static, free, lambda)<br> * 5 target function arguments<br> * N delegate subscribers<br> * Optional fixed block allocator                          | * High lines of source code<br> * Highly repetitive source code                                                                                                                                            |
| <a href="https://github.com/endurodave/AsyncMulticastDelegate">AsyncMulticastDelegate</a>             | C++03    | * Traditional template syntax<br> * Any delegate target function type (member, static, free)<br> * 5 target function arguments<br> * N delegate subscribers<br> * Optional fixed block allocator                                    | * High lines of source code<br> * Highly repetitive source code                                                                                                                                            |
| <a href="https://github.com/endurodave/AsyncCallback">AsyncCallback</a>                               | C++      | * Traditional template syntax<br> * Delegate target function type (static, free)<br> * 1 target function argument<br> * N delegate subscribers                                                                                      | * Low lines of source code<br> * Most compact C++ implementation<br> * Any C++ compiler                                                                                                                    |
| <a href="https://github.com/endurodave/C_AsyncCallback">C_AsyncCallback</a>                           | C        | * Macros provide type-safety<br> * Delegate target function type (static, free)<br> * 1 target function argument<br> * Fixed delegate subscribers (set at compile time)<br> * Optional fixed block allocator                        | * Low lines of source code<br> * Very compact implementation<br> * Any C compiler                                                                                                                          |

<p>This article documents a modern C++ implementation of asynchronous delegates. The library implements anonymous synchronous and asynchronous function callbacks. The target function is invoked with all arguments on the registrar&#39;s desired thread of control.</p>

# Introduction

<p>Nothing seems to garner the interest of C++ programmers more than delegates. In other languages, the delegate is a first-class feature so developers can use these well-understood constructs. In C++, however, a delegate is not natively available. Yet that doesn&rsquo;t stop us programmers from trying to emulate the ease with which a delegate stores and invokes any callable function.</p>

<p>Delegates normally support synchronous executions, that is, when invoked; the bound function is executed within the caller&rsquo;s thread of control. On multi-threaded applications, it would be ideal to specify the target function and the thread it should execute on without imposing function signature limitations. The library does the grunt work of getting the delegate and all argument data onto the destination thread. The idea behind this article is to provide a C++ delegate library with a consistent API that is capable of synchronous and asynchronous invocations on any callable function.</p>

<p>The features of the modern C++ delegate library are:</p>

<ol>
	<li><strong>Any Compiler</strong> &ndash; standard C++17 code for any compiler without weird hacks</li>
	<li><strong>Any Function</strong> &ndash; invoke any callable function: member, static, or free</li>
	<li><strong>Any Argument Type</strong> &ndash; supports any argument type: value, reference, pointer, pointer to pointer</li>
	<li><strong>Multiple Arguments</strong> &ndash; supports 5 number of function arguments for the bound function</li>
	<li><strong>Synchronous Invocation</strong> &ndash; call the bound function synchronously</li>
	<li><strong>Asynchronous Invocation</strong> &ndash; call the bound function asynchronously on a client specified thread</li>
	<li><strong>Blocking Asynchronous Invocation</strong> - invoke asynchronously using blocking or non-blocking delegates</li>
	<li><strong>Smart Pointer Support</strong> - bind an instance function using a raw object pointer or <code>std::shared_ptr</code></li>
    <li><strong>Lambda Support</strong> - bind and invoke lambda functions asynchronously using delegates.</li>
	<li><strong>Automatic Heap Handling</strong> &ndash; automatically copy argument data to the heap for safe transport through a message queue</li>
	<li><strong>Any OS</strong> &ndash; easy porting to any OS. C++11 <code>std::thread</code> port included</li>
	<li><strong>CMake</strong> - CMake supports most toolchains including Windows and Linux.</li>
	<li><strong>Unit Tests</strong> - extensive unit testing of the delegate library included</li>
	<li><strong>No External Libraries</strong> &ndash; delegate does not rely upon external libraries</li>
	<li><strong>Ease of Use</strong> &ndash; function signature template arguments (e.g., <code>MulticastDelegate&lt;void(TestStruct*)&gt;</code>)</li>
</ol>

<p>The delegate implementation significantly eases multithreaded application development by executing the delegate function with all of the function arguments on the thread of control that you specify. The framework handles all of the low-level machinery to safely invoke any function signature on a target thread. Windows 2017 and Eclipse projects are included for easy experimentation.</p>

# Delegates Background

<p>If you&rsquo;re not familiar with a delegate, the concept is quite simple. A delegate can be thought of as a super function pointer. In C++, there&#39;s no pointer type capable of pointing to all the possible function variations: instance member, virtual, const, static, and free (global). A function pointer can&rsquo;t point to instance member functions, and pointers to member functions have all sorts of limitations. However, delegate classes can, in a type-safe way, point to any function provided the function signature matches. In short, a delegate points to any function with a matching signature to support anonymous function invocation.</p>

<p>In practice, while a delegate is useful, a multicast version significantly expands its utility. The ability to bind more than one function pointer and sequentially invoke all registrars&rsquo; makes for an effective publisher/subscriber mechanism. Publisher code exposes a delegate container and one or more anonymous subscribers register with the publisher for callback notifications.</p>

<p>The problem with callbacks on a multithreaded system, whether it be a delegate-based or function pointer based, is that the callback occurs synchronously. Care must be taken that a callback from another thread of control is not invoked on code that isn&rsquo;t thread-safe. Multithreaded application development is hard. It&#39;s hard for the original designer; it&#39;s hard because engineers of various skill levels must maintain the code; it&#39;s hard because bugs manifest themselves in difficult ways. Ideally, an architectural solution helps to minimize errors and eases application development.</p>

<p>This C++ delegate implementation is full featured and allows calling any function, even instance member functions, with any arguments either synchronously or asynchronously. The delegate library makes binding to and invoking any function a snap.</p>

# Quick Start

A simple publish/subscribe asynchronous delegate example.

## Publisher

Typically a delegate is inserted into a delegate container. <code>AlarmCd</code> is a delegate container. 

<figure>
    <img src="Figure1.jpg" alt="Figure 1" style="width:65%;">
    <figcaption>Figure 1: AlarmCb Delegate Container</figcaption>
</figure>

<p></p>

1. <code>MulticastDelegateSafe</code> - the delegate container type.
2. <code>void(int, const string&)</code> - the function signature accepted by the delegate container. Any function matching can be inserted, such as a class member, static or lambda function.
3. <code>AlarmCb</code> - the delegate container name. 

<p>Invoke delegate container to notify subscribers.</p>

```cpp
MulticastDelegateSafe<void(int, const string&)> AlarmCb;

void NotifyAlarmSubscribers(int alarmId, const string& note)
{
    // Invoke delegate to generate callback(s) to subscribers
    AlarmCb(alarmId, note);
}
```
## Subscriber

<p>Typically a subscriber registers with a delegate container instance to receive callbacks, either synchronously or asynchronously.</p>

<figure>
    <img src="Figure2.jpg" alt="Figure 2" style="width:75%;">
    <figcaption>Figure 2: Insert into AlarmCb Delegate Container</figcaption>
</figure>

<p></p>

1. <code>AlarmCb</code> - the publisher delegate container instance.
2. <code>+=</code> - add a function target to the container. 
3. <code>MakeDelegate</code> - creates a delegate instance.
4. <code>&alarmSub</code> - the subscriber object pointer.
5. <code>&AlarmSub::MemberAlarmCb</code> - the subscriber callback member function.
6. <code>workerThread1</code> - the thread the callback will be invoked on. Adding a thread argument changes the callback type from synchronous to asynchronous.

<p>Create a function conforming to the delegate signature. Insert a callable functions into the delegate container.</p>

```cpp
class AlarmSub
{
    void AlarmSub()
    {
        // Register to receive callbacks on workerThread1
        AlarmCb += MakeDelegate(this, &AlarmSub::HandleAlarmCb, workerThread1);
    }

    void ~AlarmSub()
    {
        // Unregister from callbacks
        AlarmCb -= MakeDelegate(this, &AlarmSub::HandleAlarmCb, workerThread1);
    }

    void HandleAlarmCb(int alarmId, const string& note)
    {
        // Handle callback here. Called on workerThread1 context.
    }
}
```

<p>This is a simple example. Many other usage patterns exist including asynchronous API's, blocking delegates with a timeout, and more.</p>

# Project Build

<a href="https://www.cmake.org">CMake</a> is used to create the build files. CMake is free and open-source software. Windows, Linux and other toolchains are supported. Example CMake console commands executed inside the project root directory: 

## Windows Visual Studio

<code>cmake -G "Visual Studio 17 2022" -A Win32 -B ../AsyncMulticastDelegateCpp17Build -S .</code>

<code>cmake -G "Visual Studio 17 2022" -A x64 -B ../AsyncMulticastDelegateCpp17Build -S .</code>

<code>cmake -G "Visual Studio 17 2022" -A x64 -B ../AsyncMulticastDelegateCpp17Build -S . -DENABLE_UNIT_TESTS=ON</code>

After executed, open the Visual Studio project from within the <code>AsyncMulticastDelegateCpp17Build</code> directory.

<figure>
    <img src="Figure3.jpg" alt="Figure 3" style="width:100%;">
    <figcaption>Figure 3: Visual Studio Build</figcaption>
</figure>

## Linux Make

<code>cmake -G "Unix Makefiles" -B ../AsyncMulticastDelegateCpp17Build -S .</code>

<code>cmake -G "Unix Makefiles" -B ../AsyncMulticastDelegateCpp17Build -S . -DENABLE_UNIT_TESTS=ON</code>

After executed, build the software from within the AsyncMulticastDelegateCpp17Build directory using the command <code>make</code>. Run the console app using <code>./DelegateApp</code>.

<figure>
    <img src="Figure4.jpg" alt="Figure 4" style="width:70%;">
    <figcaption>Figure 4: Linux Makefile Build</figcaption>
</figure>

# Using the Code

<p>I&rsquo;ll first present how to use the code, and then get into the implementation details.</p>

<p>The delegate library is comprised of delegates and delegate containers. A delegate is capable of binding to a single callable function. A multicast delegate container holds one or more delegates in a list to be invoked sequentially. A single cast delegate container holds at most one delegate.</p>

<p>The primary delegate classes are listed below:</p>

<ul class="class">
	<li><code>DelegateFree&lt;&gt;</code></li>
	<li><code>DelegateFreeAsync&lt;&gt;</code></li>
	<li><code>DelegateFreeAsyncWait&lt;&gt;</code></li>
	<li><code>DelegateMember&lt;&gt;</code></li>
	<li><code>DelegateMemberAsync&lt;&gt;</code></li>
	<li><code>DelegateMemberAsyncWait&lt;&gt;</code></li>
	<li><code>DelegateMemberSp&lt;&gt;</code></li>
	<li><code>DelegateMemberSpAsync&lt;&gt;</code></li>
</ul>

<p><code>DelegateFree&lt;&gt;</code> binds to a free or static member function. <code>DelegateMember&lt;&gt; </code>binds to a class instance member function. <code>DelegateMemberSp&lt;&gt;</code> binds to a class instance member function using a <code>std::shared_ptr</code> instead of a raw object pointer. All versions offer synchronous function invocation.</p>

<p><code>DelegateFreeAsync&lt;&gt;</code>, <code>DelegateMemberAsync&lt;&gt;</code> and <code>DelegateMemberSpAsync&lt;&gt;</code> operate in the same way as their synchronous counterparts; except these versions offer non-blocking asynchronous function execution on a specified thread of control.</p>

<p><code>DelegateFreeAsyncWait&lt;&gt;</code> and <code>DelegateMemberAsyncWait&lt;&gt;</code> provides blocking asynchronous function execution on a target thread with a caller supplied maximum wait timeout.</p>

<p>The three main delegate container classes are:</p>

<ul class="class">
	<li><code>SinglecastDelegate&lt;&gt;</code></li>
	<li><code>MulticastDelegate&lt;&gt;</code></li>
	<li><code>MulticastDelegateSafe&lt;&gt;</code></li>
</ul>

<p><code>SinglecastDelegate&lt;&gt;</code> is a delegate container accepting a single delegate. The advantage of the single cast version is that it is slightly smaller and allows a return type other than <code>void</code> in the bound function.</p>

<p><code>MulticastDelegate&lt;&gt;</code> is a delegate container implemented as a singly-linked list accepting multiple delegates. Only a delegate bound to a function with a <code>void</code> return type may be added to a multicast delegate container.</p>

<p><code>MultcastDelegateSafe&lt;&gt;</code> is a thread-safe container implemented as a singly-linked list accepting multiple delegates. Always use the thread-safe version if multiple threads access the container instance.</p>

<p>Each container stores the delegate by value. This means the delegate is copied internally into either heap or fixed block memory depending on the mode. The user is not required to manually create a delegate on the heap before insertion into the container. Typically, the overloaded template function <code>MakeDelegate() </code>is used to create a delegate instance based upon the function arguments.</p>

## Synchronous Delegates

<p>All delegates are created with the overloaded <code>MakeDelegate()</code> template function. The compiler uses template argument deduction to select the correct <code>MakeDelegate()</code> version eliminating the need to manually specify the template arguments. For example, here is a simple free function.</p>

<pre lang="C++">
void FreeFuncInt(int value)
{
      cout &lt;&lt; &quot;FreeCallback &quot; &lt;&lt; value &lt;&lt; endl;
}</pre>

<p>To bind the free function to a delegate, create a <code>DelegateFree&lt;void(int)&gt;</code> instance using <code>MakeDelegate()</code>. The <code>DelegateFree </code>template argument is the complete function&#39;s signature: <code>void(int)</code>. <code>MakeDelegate()</code> returns a <code>DelegateFree&lt;void(int)&gt;</code> object and the following line invokes the function <code>FreeFuncInt</code> using the delegate.</p>

<pre lang="C++">
// Create a delegate bound to a free function then invoke
DelegateFree&lt;void(int)&gt; delegateFree = MakeDelegate(&amp;FreeFuncInt);
delegateFree(123);</pre>

<p>A member function is bound to a delegate in the same way, only this time <code>MakeDelegate()</code> uses two arguments: a class instance and a member function pointer. The two <code>DelegateMember </code>template arguments are the class name (i.e., <code>TestClass</code>) and the bound function signature (i.e. <code>void(TestStruct*)</code>).</p>

<pre lang="C++">
// Create a delegate bound to a member function then invoke    
DelegateMember&lt;TestClass, void(TestStruct*)&gt; delegateMember = 
      MakeDelegate(&amp;testClass, &amp;TestClass::MemberFunc);    
delegateMember(&amp;testStruct);</pre>

<p>Rather than create a concrete free or member delegate, typically a delegate container is used to hold one or more delegates. A delegate container can hold any delegate type. For example, a multicast delegate container that binds to any function with a <code>void (int)</code> function signature is shown below:</p>

<pre lang="C++">
MulticastDelegate&lt;void(int)&gt; delegateA;</pre>

<p>A single cast delegate is created in the same way:</p>

<pre lang="C++">
SinglecastDelegate&lt;void(int)&gt; delegateB;</pre>

<p>A function signature that returns a value is also possible. The delegate container accepts functions with one <code>float </code>argument and returns an <code>int</code>.</p>

<pre lang="C++">
SinglecastDelegate&lt;int(float)&gt; delegateC;</pre>

<p>A <code>SinglecastDelegate&lt;&gt;</code> may bind to a function that returns a value whereas a multicast versions cannot. The reason is that when multiple callbacks are invoked, which callback function return value should be used? The correct answer is none, so multicast containers only accept delegates with function signatures using <code>void </code>as the return type.</p>

<p><code>MulticastDelegate </code>containers bind to one or more functions.</p>

<pre lang="C++">
MulticastDelegate&lt;void(int, int)&gt; delegateD;

MulticastDelegate&lt;void(float, int, char)&gt; delegateE;</pre>

<p>Of course, more than just built-in pass by value argument types are supported.</p>

<pre lang="C++">
MulticastDelegate&lt;void(const MyClass&amp;, MyStruct*, Data**)&gt; delegateF;</pre>

<p>Creating a delegate instance and adding it to the multicast delegate container is accomplished with the overloaded <code>MakeDelegate()</code> function and <code>operator+=</code>. Binding a free function or <code>static</code> function only requires a single function pointer argument.</p>

<pre lang="C++">
delegateA += MakeDelegate(&amp;FreeFuncInt);</pre>

<p>An instance member function can also be added to any delegate container. For member functions, the first argument to <code>MakeDelegate()</code> is a pointer to the class instance. The second argument is a pointer to the member function.</p>

<pre lang="C++">
delegateA += MakeDelegate(&amp;testClass, &amp;TestClass::MemberFunc);</pre>

<p>Check for registered clients first, and then invoke callbacks for all registered delegates. If multiple delegates are stored within <code>MulticastDelegate&lt;void(int)&gt;</code>, each one is called sequentially.</p>

<pre lang="C++">
// Invoke the delegate target functions
if (delegateA)
      delegateA(123);</pre>

<p>Removing a delegate instance from the delegate container uses <code>operator-=</code>.</p>

<pre lang="C++">
delegateA -= MakeDelegate(&amp;FreeFuncInt);</pre>

<p>Alternatively, <code>Clear()</code> is used to remove all delegates within the container.</p>

<pre lang="C++">
delegateA.Clear();</pre>

<p>A delegate is added to the single cast container using <code>operator=</code>.</p>

<pre lang="C++">
SinglecastDelegate&lt;int(int)&gt; delegateF;
delegateF = MakeDelegate(&amp;FreeFuncIntRetInt);</pre>

<p>Removal is with <code>Clear()</code> or assigning <code>0</code>.</p>

<pre lang="C++">
delegateF.Clear();
delegateF = 0;</pre>

## Asynchronous Non-Blocking Delegates

<p>Up until this point, the delegates have all been synchronous. The asynchronous features are layered on top of the synchronous delegate implementation. To use asynchronous delegates, a thread-safe delegate container safely accessible by multiple threads is required. Locks protect the class API against simultaneous access. The &ldquo;<code>Safe</code>&rdquo; version is shown below.</p>

<pre lang="C++">
MulticastDelegateSafe&lt;void(TestStruct*)&gt; delegateC;</pre>

<p>A thread pointer as the last argument to <code>MakeDelegate()</code> forces creation of an asynchronous delegate. In this case, adding a thread argument causes <code>MakeDelegate()</code> to return a <code>DelegateMemberAsync&lt;&gt;</code> as opposed to <code>DelegateMember&lt;&gt;</code>.</p>

<pre lang="C++">
delegateC += MakeDelegate(&amp;testClass, &amp;TestClass::MemberFunc, workerThread1);</pre>

<p>Invocation is the same as the synchronous version, yet this time the callback function <code>TestClass::MemberFunc()</code> is called from <code>workerThread1</code>.</p>

<pre lang="C++">
if (delegateC)
      delegateC(&amp;testStruct);</pre>

<p>Here is another example of an asynchronous delegate being invoked on <code>workerThread1 </code>with <code>std::string</code> and <code>int </code>arguments.</p>

<pre lang="C++">
// Create delegate with std::string and int arguments then asynchronously    
// invoke on a member function
MulticastDelegateSafe&lt;void(const std::string&amp;, int)&gt; delegateH;
delegateH += MakeDelegate(&amp;testClass, &amp;TestClass::MemberFuncStdString, workerThread1);
delegateH(&quot;Hello world&quot;, 2020);</pre>

<p>Usage of the library is consistent between synchronous and asynchronous delegates. The only difference is the addition of a thread pointer argument to <code>MakeDelegate()</code>. Always remember to use the thread-safe <code>MulticastDelegateSafe&lt;&gt;</code> containers when using asynchronous delegates to callback across thread boundaries.</p>

<p>The default behavior of the delegate library when invoking non-blocking asynchronous delegates is that arguments&nbsp;are copied into heap memory for safe transport to the destination thread. This means all arguments will be duplicated. If your data is something other than plain old data (POD) and can&rsquo;t be bitwise copied, then be sure to implement an appropriate copy constructor to handle the copying yourself.</p>

<p>For more examples, see <em>main.cpp</em> and <em>DelegateUnitTests.cpp</em> within the attached source code.</p>

## Bind to std::shared_ptr

<p>Binding to instance member function requires a pointer to an object. The delegate library supports binding with a raw pointer and a <code>std::shared_ptr</code> smart pointer. Usage is what you&rsquo;d expect; just use a <code>std::shared_ptr</code> in place of the raw object pointer in the call to <code>MakeDelegate()</code>. Depending on if a thread argument is passed to <code>MakeDelegate()</code> or not, a <code>DelegateMemberSp&lt;&gt;</code> or <code>DelegateMemberSpAsync&lt;&gt;</code> instance is returned.</p>

<pre lang="C++">
// Create a shared_ptr, create a delegate, then synchronously invoke delegate function
std::shared_ptr&lt;TestClass&gt; spObject(new TestClass());
auto delegateMemberSp = MakeDelegate(spObject, &amp;TestClass::MemberFuncStdString);
delegateMemberSp(&quot;Hello world using shared_ptr&quot;, 2020);</pre>

## Caution Using Raw Object Pointers

<p>Certain asynchronous delegate usage patterns can cause a callback invocation to occur on a deleted object. The problem is this: an object function is bound to a delegate and invoked asynchronously, but before the invocation occurs on the target thread, the target object is deleted. In other words, it is possible for an object bound to a delegate to be deleted before the target thread message queue has had a chance to invoke the callback. The following code exposes the issue:</p>

<pre lang="C++">
    // Example of a bug where the testClassHeap is deleted before the asychronous delegate
    // is invoked on the workerThread1. In other words, by the time workerThread1 calls
    // the bound delegate function the testClassHeap instance is deleted and no longer valid.
    TestClass* testClassHeap = new TestClass();
    auto delegateMemberAsync = 
           MakeDelegate(testClassHeap, &amp;TestClass::MemberFuncStdString, workerThread1);
    delegateMemberAsync(&quot;Function async invoked on deleted object. Bug!&quot;, 2020);
    delegateMemberAsync.Clear();
    delete testClassHeap;</pre>

<p>The example above is contrived, but it does clearly show that nothing prevents an object being deleted while waiting for the asynchronous invocation to occur. In many embedded system architectures, the registrations might occur on singleton objects or objects that have a lifetime that spans the entire execution. In this way, the application&rsquo;s usage pattern prevents callbacks into deleted objects. However, if objects pop into existence, temporarily subscribe to a delegate for callbacks, then get deleted later the possibility of a latent delegate stuck in a message queue could invoke a function on a deleted object.</p>

<p>Fortunately, C++ smart pointers are just the ticket to solve these complex object lifetime issues. A <code>DelegateMemberSpAsync&lt;&gt;</code> delegate binds using a <code>std::shared_ptr</code> instead of a raw object pointer. Now that the delegate has a shared pointer, the danger of the object being prematurely deleted is eliminated. The shared pointer will only delete the object pointed to once all references are no longer in use. In the code snippet below, all references to <code>testClassSp </code>are removed by the client code yet the delegate&rsquo;s copy placed into the queue prevents <code>TestClass </code>deletion until after the asynchronous delegate callback occurs.</p>

<pre lang="C++">
    // Example of the smart pointer function version of the delegate. The testClassSp instance
    // is only deleted after workerThread1 invokes the callback function thus solving the bug.
    std::shared_ptr&lt;TestClass&gt; testClassSp(new TestClass());
    auto delegateMemberSpAsync = MakeDelegate
         (testClassSp, &amp;TestClass::MemberFuncStdString, workerThread1);
    delegateMemberSpAsync(&quot;Function async invoked using smart pointer. Bug solved!&quot;, 2020);
    delegateMemberSpAsync.Clear();
    testClassSp.reset();</pre>

<p>Actually, this technique can be used to call an object function, and then the object automatically deletes after the callback occurs. Using the above example, create a shared pointer instance, bind a delegate, and invoke the delegate. Now <code>testClassSp</code> can go out of scope and <code>TestClass::MemberFuncStdString</code> will still be safely called on <code>workerThread1</code>. The <code>TestClass </code>instance will delete by way of <code>std::shared_ptr&lt;TestClass&gt;</code> once the smart pointer reference count goes to 0 after the callback completes without any extra programmer involvement.</p>

<pre lang="C++">
std::shared_ptr&lt;TestClass&gt; testClassSp(new TestClass());
auto delegateMemberSpAsync =
    MakeDelegate(testClassSp, &amp;TestClass::MemberFuncStdString, workerThread1);
delegateMemberSpAsync(&quot;testClassSp deletes after delegate invokes&quot;, 2020);</pre>

## Asynchronous Blocking Delegates

<p>A blocking delegate waits until the target thread executes the bound delegate function. Unlike non-blocking delegates, the blocking versions do not copy argument data onto the heap. They also allow function return types other than <code>void</code> whereas the non-blocking delegates only bind to functions returning <code>void</code>. Since the function arguments are passed to the destination thread unmodified, the function executes just as you&#39;d expect a synchronous version including incoming/outgoing pointers and references.</p>

<p>Stack arguments passed by pointer/reference need not be thread-safe. The reason is that the calling thread blocks waiting for the destination thread to complete. This means that the delegate implementation guarantees only one thread is able to access stack allocated argument data.</p>

<p>A blocking delegate must specify a timeout in milliseconds or <code>WAIT_INFINITE</code>. Unlike a non-blocking asynchronous delegate, which is guaranteed to be invoked, if the timeout expires on a blocking delegate, the function is not invoked. Use <code>IsSuccess()</code> to determine if the delegate succeeded or not.</p>

<p>Adding a timeout as the last argument to <code>MakeDelegate()</code> causes a <code>DelegateFreeAsyncWait&lt;&gt;</code> or <code>DelegateMemberAsyncWait&lt;&gt;</code> instance to be returned depending on if a free or member function is being bound. A &quot;<code>Wait</code>&quot; delegate is typically not added to a delegate container. The typical usage pattern is to create a delegate and function arguments on the stack, then invoke. The code fragment below creates a blocking delegate with the function signature <code>int (std::string&amp;</code>). The function is called on <code>workerThread1</code>. The function <code>MemberFuncStdStringRetInt()</code> will update the outgoing <code>string msg</code> and return an integer to the caller.</p>

<pre lang="C++">
    // Create a asynchronous blocking delegate and invoke. This thread will block until the
    // msg and year stack values are set by MemberFuncStdStringRetInt on workerThread1.
    auto delegateI = 
          MakeDelegate(&amp;testClass, &amp;TestClass::MemberFuncStdStringRetInt, 
                       workerThread1, WAIT_INFINITE);
    std::string msg;
    int year = delegateI(msg);
    if (delegateI.IsSuccess())
    {
        cout &lt;&lt; msg.c_str() &lt;&lt; &quot; &quot; &lt;&lt; year &lt;&lt; endl;
    }</pre>

## Asynchronous Lambda Invocation

<p>Delegates can invoke non-capturing&nbsp;lambda functions asynchronously. The example below calls&nbsp;<code>LambdaFunc1&nbsp;</code>on&nbsp;<code>workerThread1</code>.&nbsp;</p>

<pre data-allowshrink="True" data-codeblock-processed="true" data-collapse="False" id="pre516246">
auto LambdaFunc1 = +[](int i) -&gt; int
{
    cout &lt;&lt; &quot;Called LambdaFunc1 &quot; &lt;&lt; i &lt;&lt; std::endl;
    return ++i;
};

// Asynchronously invoke lambda on workerThread1 and wait for the return value
auto lambdaDelegate1 = MakeDelegate(LambdaFunc1, workerThread1, WAIT_INFINITE);
int lambdaRetVal2 = lambdaDelegate1(123);
</pre>

<p>Delegates are callable and therefore may be passed to the standard library. The example below shows&nbsp;<code>CountLambda&nbsp;</code>executed asynchronously on&nbsp;<code>workerThread1&nbsp;</code>by&nbsp;<code>std::count_if</code>.</p>

<pre data-allowshrink="True" data-codeblock-processed="true" data-collapse="False" id="pre407028">
std::vector&lt;int&gt; v{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

auto CountLambda = +[](int v) -&gt; int
{
    return v &gt; 2 &amp;&amp; v &lt;= 6;
};
auto countLambdaDelegate = MakeDelegate(CountLambda, workerThread1, WAIT_INFINITE);

const auto valAsyncResult = std::count_if(v.begin(), v.end(),
    countLambdaDelegate);
cout &lt;&lt; &quot;Asynchronous lambda result: &quot; &lt;&lt; valAsyncResult &lt;&lt; endl;</pre>

# Delegate Library

<p>The delegate library contains numerous classes. A single include <em>DelegateLib.h</em> provides access to all delegate library features. The library is wrapped within a <code>DelegateLib </code>namespace. Included unit tests help ensure a robust implementation. The table below shows the delegate class hierarchy.</p>

```cpp
DelegateBase
    Delegate<>
        DelegateFree<>
            DelegateFreeAsync<>
                DelegateFreeAsyncWait<>
        DelegateMember<>
            DelegateMemberAsync<>
                DelegateMemberAsyncWait<>
        DelegateMemberSp<>
```

For this implementation the section "Delegate Library" was not updated. See the section "Delegate Library" in both the links below for design implementation details. While some information (like code snippets) are not directly related to this implementation, the ideas and cautions are still useful. 

* <a href="https://github.com/endurodave/AsyncMulticastDelegateCpp11">AsyncMulticastDelegateCpp11</a><br>
* <a href="https://github.com/endurodave/AsyncMulticastDelegateModern">AsyncMulticastDelegateModern</a><br>

## Worker Thread (std::thread)

<p>The <code>std::thread</code> implemented thread loop is shown below. The loop calls the <code>DelegateInvoke()</code>&nbsp;function on each asynchronous delegate instance.</p>

<pre lang="C++">
void WorkerThread::Process()
{
    while (1)
    {
        std::shared_ptr&lt;ThreadMsg&gt; msg;
        {
            // Wait for a message to be added to the queue
            std::unique_lock&lt;std::mutex&gt; lk(m_mutex);
            while (m_queue.empty())
                m_cv.wait(lk);

            if (m_queue.empty())
                continue;

            msg = m_queue.front();
            m_queue.pop();
        }

        switch (msg-&gt;GetId())
        {
            case MSG_DISPATCH_DELEGATE:
            {
                ASSERT_TRUE(msg-&gt;GetData() != NULL);

                // Convert the ThreadMsg void* data back to a DelegateMsg* 
                auto delegateMsg = msg-&gt;GetData();

                // Invoke the callback on the target thread
                delegateMsg-&gt;GetDelegateInvoker()-&gt;DelegateInvoke(delegateMsg);
                break;
            }

            case MSG_EXIT_THREAD:
            {
                return;
            }

            default:
                ASSERT();
        }
    }
}</pre>

<p>Any project-specific thread loop can call <code>DelegateInvoke()</code>. This is just one example. The only requirement is that your worker thread class inherit from&nbsp;<code>DelegateLib::DelegateThread</code> and implement the&nbsp;<code>DispatchDelegate()</code> abstract function. <code>DisplatchDelegate()</code> will insert the shared message pointer into the thread queue for processing.&nbsp;</p>

# Delegate Containers

<p>Delegate containers store one or more delegates. The delegate container hierarchy is shown below:</p>

```cpp
MulticastDelegate<>
    MulticastDelegateSafe<>
SinglecastDelegate<>
```

<p><code>MulticastDelegate&lt;&gt;</code> provides the function <code>operator()</code> to sequentially invoke each delegate within the list.</p>

<p><code>MulticastDelegateSafe&lt;&gt;</code> provides a thread-safe wrapper around the delegate API. Each function provides a lock guard to protect against simultaneous access. The Resource Acquisition is Initialization (RAII) technique is used for the locks.</p>

<pre lang="C++">
template &lt;class R&gt;
struct MulticastDelegateSafe; // Not defined

/// @brief Thread-safe multicast delegate container class. 
template&lt;class RetType, class... Args&gt;
class MulticastDelegateSafe&lt;RetType(Args...)&gt; : public MulticastDelegate&lt;RetType(Args...)&gt;
{
public:
    MulticastDelegateSafe() { LockGuard::Create(&amp;m_lock); }
    ~MulticastDelegateSafe() { LockGuard::Destroy(&amp;m_lock); }

    void operator+=(const Delegate&lt;RetType(Args...)&gt;&amp; delegate) {
        LockGuard lockGuard(&amp;m_lock);
        MulticastDelegate&lt;RetType(Args...)&gt;::operator +=(delegate);
    }
    void operator-=(const Delegate&lt;RetType(Args...)&gt;&amp; delegate) {
        LockGuard lockGuard(&amp;m_lock);
        MulticastDelegate&lt;RetType(Args...)&gt;::operator -=(delegate);
    }
    void operator()(Args... args) {
        LockGuard lockGuard(&amp;m_lock);
        MulticastDelegate&lt;RetType(Args...)&gt;::operator ()(args...);
    }
    bool Empty() {
        LockGuard lockGuard(&amp;m_lock);
        return MulticastDelegate&lt;RetType(Args...)&gt;::Empty();
    }
    void Clear() {
        LockGuard lockGuard(&amp;m_lock);
        MulticastDelegate&lt;RetType(Args...)&gt;::Clear();
    }

    explicit operator bool() {
        LockGuard lockGuard(&amp;m_lock);
        return MulticastDelegate&lt;RetType(Args...)&gt;::operator bool();
    }

private:
    // Prevent copying objects
    MulticastDelegateSafe(const MulticastDelegateSafe&amp;) = delete;
    MulticastDelegateSafe&amp; operator=(const MulticastDelegateSafe&amp;) = delete;

    /// Lock to make the class thread-safe
    LOCK m_lock;
};</pre>

# Examples

## SysData Example

<p>A few real-world examples will demonstrate common delegate usage patterns. First, <code>SysData </code>is a simple class showing how to expose an outgoing asynchronous interface. The class stores system data and provides asynchronous subscriber notifications when the mode changes. The class interface is shown below:</p>

<pre lang="C++">
class SysData
{
public:
    /// Clients register with MulticastDelegateSafe1 to get callbacks when system mode changes
    MulticastDelegateSafe&lt;void(const SystemModeChanged&amp;)&gt; SystemModeChangedDelegate;

    /// Get singleton instance of this class
    static SysData&amp; GetInstance();

    /// Sets the system mode and notify registered clients via SystemModeChangedDelegate.
    /// @param[in] systemMode - the new system mode. 
    void SetSystemMode(SystemMode::Type systemMode);    

private:
    SysData();
    ~SysData();

    /// The current system mode data
    SystemMode::Type m_systemMode;

    /// Lock to make the class thread-safe
    LOCK m_lock;
};</pre>

<p>The subscriber interface for receiving callbacks is <code>SystemModeChangedDelegate</code>. Calling <code>SetSystemMode()</code> saves the new mode into <code>m_systemMode </code>and notifies all registered subscribers.</p>

<pre lang="C++">
void SysData::SetSystemMode(SystemMode::Type systemMode)
{
    LockGuard lockGuard(&amp;m_lock);

    // Create the callback data
    SystemModeChanged callbackData;
    callbackData.PreviousSystemMode = m_systemMode;
    callbackData.CurrentSystemMode = systemMode;

    // Update the system mode
    m_systemMode = systemMode;

    // Callback all registered subscribers
    if (SystemModeChangedDelegate)
        SystemModeChangedDelegate(callbackData);
}</pre>

## SysDataClient Example

<p><code>SysDataClient </code>is a delegate subscriber and registers for <code>SysData::SystemModeChangedDelegate</code> notifications within the constructor.</p>

<pre lang="C++">
    // Constructor
    SysDataClient() :
        m_numberOfCallbacks(0)
    {
        // Register for async delegate callbacks
        SysData::GetInstance().SystemModeChangedDelegate += 
                 MakeDelegate(this, &amp;SysDataClient::CallbackFunction, workerThread1);
        SysDataNoLock::GetInstance().SystemModeChangedDelegate += 
                       MakeDelegate(this, &amp;SysDataClient::CallbackFunction, workerThread1);
    }</pre>

<p><code>SysDataClient::CallbackFunction()</code> is now called on <code>workerThread1 </code>when the system mode changes.</p>

<pre lang="C++">
    void CallbackFunction(const SystemModeChanged&amp; data)
    {
        m_numberOfCallbacks++;
        cout &lt;&lt; &quot;CallbackFunction &quot; &lt;&lt; data.CurrentSystemMode &lt;&lt; endl;
    }</pre>

<p>When <code>SetSystemMode()</code> is called, anyone interested in the mode changes are notified synchronously or asynchronously depending on the delegate type registered.</p>

<pre lang="C++">
// Set new SystemMode values. Each call will invoke callbacks to all
// registered client subscribers.
SysData::GetInstance().SetSystemMode(SystemMode::STARTING);
SysData::GetInstance().SetSystemMode(SystemMode::NORMAL);</pre>

## SysDataNoLock Example

<p><code>SysDataNoLock</code> is an alternate implementation that uses a <code>private</code> <code>MulticastDelegateSafe&lt;&gt;</code> for setting the system mode asynchronously and without locks.</p>

<pre lang="C++">
class SysDataNoLock
{
public:
    /// Clients register with MulticastDelegateSafe to get callbacks when system mode changes
    MulticastDelegateSafe&lt;void(const SystemModeChanged&amp;)&gt; SystemModeChangedDelegate;

    /// Get singleton instance of this class
    static SysDataNoLock&amp; GetInstance();

    /// Sets the system mode and notify registered clients via SystemModeChangedDelegate.
    /// @param[in] systemMode - the new system mode. 
    void SetSystemMode(SystemMode::Type systemMode);    

    /// Sets the system mode and notify registered clients via a temporary stack created
    /// asynchronous delegate. 
    /// @param[in] systemMode - The new system mode. 
    void SetSystemModeAsyncAPI(SystemMode::Type systemMode);    

    /// Sets the system mode and notify registered clients via a temporary stack created
    /// asynchronous delegate. This version blocks (waits) until the delegate callback
    /// is invoked and returns the previous system mode value. 
    /// @param[in] systemMode - The new system mode. 
    /// @return The previous system mode. 
    SystemMode::Type SetSystemModeAsyncWaitAPI(SystemMode::Type systemMode);

private:
    SysDataNoLock();
    ~SysDataNoLock();

    /// Private callback to get the SetSystemMode call onto a common thread
    MulticastDelegateSafe&lt;void(SystemMode::Type)&gt; SetSystemModeDelegate; 

    /// Sets the system mode and notify registered clients via SystemModeChangedDelegate.
    /// @param[in] systemMode - the new system mode. 
    void SetSystemModePrivate(SystemMode::Type);    

    /// The current system mode data
    SystemMode::Type m_systemMode;
};</pre>

<p>The constructor registers <code>SetSystemModePrivate()</code> with the <code>private</code> <code>SetSystemModeDelegate</code>.</p>

<pre lang="C++">
SysDataNoLock::SysDataNoLock() :
    m_systemMode(SystemMode::STARTING)
{
    SetSystemModeDelegate += MakeDelegate
                 (this, &amp;SysDataNoLock::SetSystemModePrivate, workerThread2);
    workerThread2.CreateThread();
}</pre>

<p>The <code>SetSystemMode()</code> function below is an example of an asynchronous incoming interface. To the caller, it looks like a normal function, but under the hood, a private member call is invoked asynchronously using a delegate. In this case, invoking <code>SetSystemModeDelegate</code> causes <code>SetSystemModePrivate()</code> to be called on <code>workerThread2</code>.</p>

<pre lang="C++">
void SysDataNoLock::SetSystemMode(SystemMode::Type systemMode)
{
    // Invoke the private callback. SetSystemModePrivate() will be called on workerThread2.
    SetSystemModeDelegate(systemMode);
}</pre>

<p>Since this <code>private</code> function is always invoked asynchronously on <code>workerThread2</code>, it doesn&#39;t require locks.</p>

<pre lang="C++">
void SysDataNoLock::SetSystemModePrivate(SystemMode::Type systemMode)
{
      // Create the callback data
      SystemModeChanged callbackData;

      callbackData.PreviousSystemMode = m_systemMode;
      callbackData.CurrentSystemMode = systemMode;

      // Update the system mode
      m_systemMode = systemMode;

      // Callback all registered subscribers
      if (SystemModeChangedDelegate)
            SystemModeChangedDelegate(callbackData);
}</pre>

## SysDataNoLock Reinvoke Example

<p>While creating a separate <code>private</code> function to create an asynchronous API does work, with delegates, it&#39;s possible to just reinvoke the same exact function just on a different thread. Perform a simple check whether the caller is executing on the desired thread of control. If not, a temporary asynchronous delegate is created on the stack and then invoked. The delegate and all the caller&rsquo;s original function arguments are duplicated on the heap and the function is reinvoked on <code>workerThread2</code>. This is an elegant way to create asynchronous APIs with the absolute minimum of effort.</p>

<pre lang="C++">
void SysDataNoLock::SetSystemModeAsyncAPI(SystemMode::Type systemMode)
{
    // Is the caller executing on workerThread2?
    if (workerThread2.GetThreadId() != WorkerThread::GetCurrentThreadId())
    {
        // Create an asynchronous delegate and re-invoke the function call on workerThread2
        auto delegate = 
             MakeDelegate(this, &amp;SysDataNoLock::SetSystemModeAsyncAPI, workerThread2);
        delegate(systemMode);
        return;
    }

    // Create the callback data
    SystemModeChanged callbackData;
    callbackData.PreviousSystemMode = m_systemMode;
    callbackData.CurrentSystemMode = systemMode;

    // Update the system mode
    m_systemMode = systemMode;

    // Callback all registered subscribers
    if (SystemModeChangedDelegate)
        SystemModeChangedDelegate(callbackData);
}</pre>

## SysDataNoLock Blocking Reinvoke Example

<p>A blocking asynchronous API can be hidden inside a class member function. The function below sets the current mode on <code>workerThread2 </code>and returns the previous mode. A blocking delegate is created on the stack and invoked if the caller isn&#39;t executing on <code>workerThread2</code>. To the caller, the function appears synchronous, but the delegate ensures that the call is executed on the proper thread before returning.</p>

<pre lang="C++">
SystemMode::Type SysDataNoLock::SetSystemModeAsyncWaitAPI(SystemMode::Type systemMode)
{
    // Is the caller executing on workerThread2?
    if (workerThread2.GetThreadId() != WorkerThread::GetCurrentThreadId())
    {
        // Create an asynchronous delegate and re-invoke the function call on workerThread2
        auto delegate =
            MakeDelegate(this, &amp;SysDataNoLock::SetSystemModeAsyncWaitAPI, 
                         workerThread2, WAIT_INFINITE);
        return delegate(systemMode);
    }

    // Create the callback data
    SystemModeChanged callbackData;
    callbackData.PreviousSystemMode = m_systemMode;
    callbackData.CurrentSystemMode = systemMode;

    // Update the system mode
    m_systemMode = systemMode;

    // Callback all registered subscribers
    if (SystemModeChangedDelegate)
        SystemModeChangedDelegate(callbackData);

    return callbackData.PreviousSystemMode;
}</pre>

## Timer Example

<p>Once a delegate framework is in place, creating a timer callback service is trivial. Many systems need a way to generate a callback based on a timeout. Maybe it&#39;s a periodic timeout for some low speed polling or maybe an error timeout in case something doesn&#39;t occur within the expected time frame. Either way, the callback must occur on a specified thread of control. A <code>SinglecastDelegate&lt;void(void)&gt; </code>used inside a <code>Timer</code> class solves this nicely.</p>

<pre lang="C++">
/// @brief A timer class provides periodic timer callbacks on the client&#39;s&nbsp;
/// thread of control. Timer is thread safe.
class Timer&nbsp;
{
public:
&nbsp;&nbsp; &nbsp;static const DWORD MS_PER_TICK;

&nbsp;&nbsp; &nbsp;/// Client&#39;s register with Expired to get timer callbacks
&nbsp;&nbsp; &nbsp;SinglecastDelegate&lt;void(void)&gt; Expired;

&nbsp;&nbsp; &nbsp;/// Constructor
&nbsp;&nbsp; &nbsp;Timer(void);

&nbsp;&nbsp; &nbsp;/// Destructor
&nbsp;&nbsp; &nbsp;~Timer(void);</pre>

<p>Users create an instance of the timer and register for the expiration. In this case, <code>MyClass::MyCallback() </code>is called in 1000ms.</p>

<pre lang="C++">
m_timer.Expired = MakeDelegate(&amp;myClass, &amp;MyClass::MyCallback, myThread);
m_timer.Start(1000);</pre>

# Summary

<p>All delegates can be created with <code>MakeDelegate()</code>. The function arguments determine the delegate type returned.</p>

<p>Synchronous delegates are created using one argument for free functions and two for instance member functions.</p>

<pre lang="C++">
auto freeDelegate = MakeDelegate(&amp;MyFreeFunc);
auto memberDelegate = MakeDelegate(&amp;myClass, &amp;MyClass::MyMemberFunc);</pre>

<p>Adding the thread argument creates a non-blocking asynchronous delegate.</p>

<pre lang="C++">
auto freeDelegate = MakeDelegate(&amp;MyFreeFunc, myThread);
auto memberDelegate = MakeDelegate(&amp;myClass, &amp;MyClass::MyMemberFunc, myThread);</pre>

<p>A <code>std::shared_ptr</code> can replace a raw instance pointer on synchronous and non-blocking asynchronous member delegates.</p>

<pre lang="C++">
std::shared_ptr&lt;MyClass&gt; myClass(new MyClass());
auto memberDelegate = MakeDelegate(myClass, &amp;MyClass::MyMemberFunc, myThread);</pre>

<p>Adding a <code>timeout</code> argument creates a blocking asynchronous delegate.</p>

<pre lang="C++">
auto freeDelegate = MakeDelegate(&amp;MyFreeFunc, myThread, WAIT_INFINITE);
auto memberDelegate = MakeDelegate(&amp;myClass, &amp;MyClass::MyMemberFunc, myThread, 5000);</pre>

<p>Delegates are added/removed from multicast containers using <code>operator+=</code> and <code>operator-=</code>. All containers accept all delegate types.</p>

<pre lang="C++">
MulticastDelegate&lt;void(int)&gt; multicastContainer;
multicastContainer += MakeDelegate(&amp;MyFreeFunc);
multicastContainer -= MakeDelegate(&amp;MyFreeFunc);</pre>

<p>Use the thread-safe multicast delegate container when using asynchronous delegates to allow multiple threads to safely add/remove from the container.</p>

<pre lang="C++">
MulticastDelegateSafe&lt;void(int)&gt; multicastContainer;
multicastContainer += MakeDelegate(&amp;MyFreeFunc, myThread);
multicastContainer -= MakeDelegate(&amp;MyFreeFunc, myThread);</pre>

<p>Single cast delegates are added and removed using <code>operator=</code>.</p>

<pre lang="C++">
SinglecastDelegate&lt;void(int)&gt; singlecastContainer;
singlecastContainer = MakeDelegate(&amp;MyFreeFunc);
singlecastContainer = 0;</pre>

<p>All delegates and delegate containers are invoked using <code>operator()</code>.</p>

<pre lang="C++">
if (myDelegate)
      myDelegate(123);</pre>

<p>Use <code>IsSuccess()</code> on blocking delegates before using the return value or outgoing arguments.</p>

<pre lang="C++">
if (myDelegate) 
{
     int outInt = 0;
     int retVal = myDelegate(&amp;outInt);
     if (myDelegate.IsSuccess()) 
     {
          cout &lt;&lt; outInt &lt;&lt; retVal;
     }
}</pre>

# Which Callback Implementation?

<p>I&rsquo;ve documented four different asynchronous multicast callback implementations here on CodeProject. Each version has its own unique features and advantages. The sections below highlight the main differences between each solution. See the <strong>References </strong>section below for links to each article.</p>

## Asynchronous Multicast Callbacks in C

<ul>
	<li>Implemented in C</li>
	<li>Callback function is a free or static member only</li>
	<li>One callback argument supported</li>
	<li>Callback argument must be a pointer type</li>
	<li>Callback argument data copied with <code>memcpy</code></li>
	<li>Type-safety provided by macros</li>
	<li>Static array holds registered subscriber callbacks</li>
	<li>Number of registered subscribers fixed at compile time</li>
	<li>Fixed block memory allocator in C</li>
	<li>Compact implementation</li>
</ul>

## Asynchronous Multicast Callbacks with Inter-Thread Messaging

<ul>
	<li>Implemented in C++</li>
	<li>Callback function is a free or static member only</li>
	<li>One callback argument supported</li>
	<li>Callback argument must be a pointer type</li>
	<li>Callback argument data copied with copy constructor</li>
	<li>Type-safety provided by templates</li>
	<li>Minimal use of templates</li>
	<li>Dynamic list of registered subscriber callbacks</li>
	<li>Number of registered subscribers expands at runtime</li>
	<li>Fixed block memory allocator in C++</li>
	<li>Compact implementation</li>
</ul>

## Asynchronous Multicast Delegates in C++

<ul>
	<li>Implemented in C++</li>
	<li>C++ delegate paradigm</li>
	<li>Any callback function type (member, static, free)</li>
	<li>Multiple callback arguments supported (up to 5)</li>
	<li>Callback argument any type (value, reference, pointer, pointer to pointer)</li>
	<li>Callback argument data copied with copy constructor</li>
	<li>Type-safety provided by templates</li>
	<li>Heavy use of templates</li>
	<li>Dynamic list of registered subscriber callbacks</li>
	<li>Number of registered subscribers expands at runtime</li>
	<li>Fixed block memory allocator in C++</li>
	<li>Larger implementation</li>
</ul>

## Asynchronous Multicast Delegates in Modern C++

<ul>
	<li>Implemented in C++ (i.e., C++17)</li>
	<li>C++ delegate paradigm</li>
	<li>Function signature delegate arguments</li>
	<li>Any callback function type (member, static, free)</li>
	<li>Multiple callback arguments supported (N arguments supported)</li>
	<li>Callback argument any type (value, reference, pointer, pointer to pointer)</li>
	<li>Callback argument data copied with copy constructor</li>
	<li>Type-safety provided by templates</li>
	<li>Heavy use of templates</li>
	<li>Variadic templates</li>
	<li>Template metaprogramming</li>
	<li>Dynamic list of registered subscriber callbacks</li>
	<li>Number of registered subscribers expands at runtime</li>
	<li>Compact implementation (due to variadic templates)</li>
</ul>

# Limitations

<p><a href="https://www.codeproject.com/Articles/5262271/Remote-Procedure-Calls-using-Cplusplus-Delegates">Remote delegates</a> that invoke a function located in a separate process or CPU are not currently supported by the delegate library.</p>

<p>A fixed block allocator is not currently supported. All dynamic memory is obtained from the heap using <code>operator new</code> and <code>delete</code>.</p>

# References

<ul>
	<li><strong><a href="https://www.codeproject.com/Articles/1160934/Asynchronous-Multicast-Delegates-in-Cplusplus">Asynchronous Multicast Delegates in C++</a></strong> - by David Lafreniere</li>
	<li><a href="https://www.codeproject.com/Articles/5262271/Remote-Procedure-Calls-using-Cplusplus-Delegates"><strong>Remote Procedure Calls using C++ Delegates</strong></a> - by David Lafreniere</li>
	<li><a href="https://www.codeproject.com/Articles/1272894/Asynchronous-Multicast-Callbacks-in-C"><strong>Asynchronous Multicast Callbacks in C</strong></a> - by David Lafreniere</li>
	<li><a href="https://www.codeproject.com/Articles/1092727/Asynchronous-Multicast-Callbacks-with-Inter-Thread"><strong>Asynchronous Multicast Callbacks with Inter-Thread Messaging</strong></a> - by David Lafreniere</li>
	<li><a href="https://www.codeproject.com/Articles/1191232/Type-Safe-Multicast-Callbacks-in-C"><strong>Type-Safe Multicast Callbacks in C</strong></a> - by David Lafreniere</li>
	<li><a href="https://www.codeproject.com/Articles/1169105/Cplusplus-std-thread-Event-Loop-with-Message-Queue"><strong>C++ std::thread Event Loop with Message Queue and Timer</strong></a> - by David Lafreniere</li>
</ul>

# Conclusion

<p>I&rsquo;ve done quite a bit of multithreaded application development over the years. Invoking a function on a destination thread with data has always been a hand-crafted, time consuming process. This library generalizes those constructs and encapsulates them into a user-friendly delegate library.</p>

<p>The article proposes a modern C++ multicast delegate implementation supporting synchronous and asynchronous function invocation. Non-blocking asynchronous delegates offer fire-and-forget invocation whereas the blocking versions allow waiting for a return value and outgoing reference arguments from the target thread. Multicast delegate containers expand the delegate&rsquo;s usefulness by allowing multiple clients to register for callback notification. Multithreaded application development is simplified by letting the library handle the low-level threading details of invoking functions and moving data across thread boundaries. The inter-thread code is neatly hidden away within the library and users only interact with an easy to use delegate API.</p>






