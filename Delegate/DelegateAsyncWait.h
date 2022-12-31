#ifndef _DELEGATE_ASYNC_WAIT_H
#define _DELEGATE_ASYNC_WAIT_H

// DelegateAsyncWait.h
// @see https://www.codeproject.com/Articles/1160934/Asynchronous-Multicast-Delegates-in-Cplusplus
// David Lafreniere, Oct 2022.

#include "Delegate.h"
#include "IDelegateThread.h"
#include "DelegateInvoker.h"
#include "Semaphore.h"
#include <memory>
#include <chrono>
#include <optional>

/// @brief Asynchronous member delegate that invokes the target function on the specified thread of control
/// and waits for the function to be executed or a timeout occurs. Use IsSuccess() to determine if asynchronous 
/// call succeeded.

namespace DelegateLib {

#undef max
constexpr auto WAIT_INFINITE = std::chrono::milliseconds::max();

template <class C, class R>
struct DelegateMemberAsyncWaitInvoke; // Not defined

// Member function with void return type
template <class TClass, class... Args>
class DelegateMemberAsyncWaitInvoke<TClass, void(Args...)> {
public:
	void operator()(DelegateMember<TClass, void(Args...)>* instance,
		Args... args) {
		(*instance)(args...);
	}

	void GetRetVal() { }
};

// Member function with non-void return type
template <class TClass, class RetType, class... Args>
class DelegateMemberAsyncWaitInvoke<TClass, RetType(Args...)> {
public:
	void operator()(DelegateMember<TClass, RetType(Args...)>* instance,
		Args... args) {
		m_retVal = (*instance)(args...);
	}

	RetType GetRetVal() { return m_retVal; }
private:
	RetType m_retVal;
};

template <class R>
struct DelegateFreeAsyncWaitInvoke; // Not defined

// Free function with void return type
template <class... Args>
class DelegateFreeAsyncWaitInvoke<void(Args...)> {
public:
	void operator()(DelegateFree<void(Args...)>* instance,
		Args... args) {
		(*instance)(args...);
	}

	void GetRetVal() { }
};

// Free function with non-void return type
template <class RetType, class... Args>
class DelegateFreeAsyncWaitInvoke<RetType(Args...)> {
public:
	void operator()(DelegateFree<RetType(Args...)>* instance,
		Args... args) {
		m_retVal = (*instance)(args...);
	}

	RetType GetRetVal() { return m_retVal; }
private:
	RetType m_retVal;
};

template <class R>
struct DelegateFreeAsyncWait; // Not defined

template <class RetType, class... Args>
class DelegateFreeAsyncWait<RetType(Args...)> : public DelegateFree<RetType(Args...)>, public IDelegateInvoker {
public:
    typedef std::integral_constant<std::size_t, sizeof...(Args)> ArgCnt;
    typedef RetType(*FreeFunc)(Args...);
    using ClassType = DelegateFreeAsyncWait<RetType(Args...)>;
    using BaseType = DelegateFree<RetType(Args...)>;

    // Contructors take a free function, delegate thread and timeout
    DelegateFreeAsyncWait(FreeFunc func, DelegateThread& thread, std::chrono::milliseconds timeout) :
        BaseType(func), m_thread(thread), m_timeout(timeout) {
        Bind(func, thread);
    }
    DelegateFreeAsyncWait(const DelegateFreeAsyncWait& rhs) : BaseType(rhs), m_thread(rhs.m_thread), m_sync(false) {
        Swap(rhs);
    }
    DelegateFreeAsyncWait() = delete;

    virtual ClassType* Clone() const override {
        return new ClassType(*this);
    }

    /// Bind a free function to a delegate. 
    void Bind(FreeFunc func, DelegateThread& thread) {
        m_thread = thread;
        BaseType::Bind(func);
    }

    virtual bool operator==(const DelegateBase& rhs) const override {
        auto derivedRhs = dynamic_cast<const ClassType*>(&rhs);
        return derivedRhs &&
            &m_thread == &derivedRhs->m_thread &&
            BaseType::operator==(rhs);
    }

    DelegateFreeAsyncWait& operator=(const DelegateFreeAsyncWait& rhs) {
        if (&rhs != this) {
            BaseType::operator=(rhs);
            Swap(rhs);
        }
        return *this;
    }

    /// Invoke delegate function asynchronously
    virtual RetType operator()(Args... args) override {
        if (m_sync)
            return BaseType::operator()(args...);
        else
        {
            // Create a clone instance of this delegate 
            auto delegate = std::shared_ptr<ClassType>(Clone());

            static_assert(ArgCnt::value <= 5, "Maximum arguments exceeded");
            if constexpr (ArgCnt::value == 0)
            {
                auto msg = std::make_shared<DelegateMsgBase>(delegate);
                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 1)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);

                using Param1 = ArgTypeOf<0, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                auto msg = std::make_shared<DelegateMsg1<Param1>>(delegate, heap_p1);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 2)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                auto msg = std::make_shared<DelegateMsg2<Param1, Param2>>(delegate, heap_p1, heap_p2);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 3)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                auto msg = std::make_shared<DelegateMsg3<Param1, Param2, Param3>>(delegate, heap_p1, heap_p2, heap_p3);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 4)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);
                decltype(auto) p4 = ArgValueOf<3>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;
                using Param4 = ArgTypeOf<3, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                decltype(auto) heap_p4 = DelegateParam<Param4>::New(p4);
                auto msg = std::make_shared<DelegateMsg4<Param1, Param2, Param3, Param4>>(delegate, heap_p1, heap_p2, heap_p3, heap_p4);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 5)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);
                decltype(auto) p4 = ArgValueOf<3>(args...);
                decltype(auto) p5 = ArgValueOf<4>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;
                using Param4 = ArgTypeOf<3, Args...>;
                using Param5 = ArgTypeOf<4, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                decltype(auto) heap_p4 = DelegateParam<Param4>::New(p4);
                decltype(auto) heap_p5 = DelegateParam<Param5>::New(p5);
                auto msg = std::make_shared<DelegateMsg5<Param1, Param2, Param3, Param4, Param5>>(delegate, heap_p1, heap_p2, heap_p3, heap_p4, heap_p5);

                m_thread.DispatchDelegate(msg);
            }

            // Wait for target thread to execute the delegate target function
            if ((m_success = delegate->m_sema.Wait(m_timeout)))
                m_invoke = delegate->m_invoke;

            return m_invoke.GetRetVal();
        }
    }

    /// Invoke delegate function asynchronously
    auto AsyncInvoke(Args... args)
    {
        if constexpr (std::is_void<RetType>::value == true)
        {
            operator()(args...);
            return IsSuccess() ? std::optional<bool>(true) : std::optional<bool>();
        }
        else
        {
            auto retVal = operator()(args...);
            return IsSuccess() ? std::optional<RetType>(retVal) : std::optional<RetType>();
        }
    }

    /// Called by the target thread to invoke the delegate function 
    virtual void DelegateInvoke(std::shared_ptr<DelegateMsgBase> msg) override {
        m_sync = true;

        static_assert(ArgCnt::value <= 5, "Maximum arguments exceeded");
        if constexpr (ArgCnt::value == 0)
        {
            m_invoke(this);
        }
        else if constexpr (ArgCnt::value == 1)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg1<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            m_invoke(this, p1);
    }
        else if constexpr (ArgCnt::value == 2)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg2<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            m_invoke(this, p1, p2);
        }
        else if constexpr (ArgCnt::value == 3)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg3<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            auto p3 = delegateMsg->GetParam3();
            m_invoke(this, p1, p2, p3);
        }
        else if constexpr (ArgCnt::value == 4)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg4<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            auto p3 = delegateMsg->GetParam3();
            auto p4 = delegateMsg->GetParam4();
            m_invoke(this, p1, p2, p3, p4);
        }
        else if constexpr (ArgCnt::value == 5)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg5<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            auto p3 = delegateMsg->GetParam3();
            auto p4 = delegateMsg->GetParam4();
            auto p5 = delegateMsg->GetParam5();
            m_invoke(this, p1, p2, p3, p4, p5);
        }

        // Signal the waiting thread
        m_sema.Signal();
    }

    /// Returns true if asynchronous function successfully invoked on target thread
    bool IsSuccess() { return m_success; }

    /// Returns the async function return value
    RetType GetRetVal() { return m_invoke.GetRetVal(); }

private:
    void Swap(const DelegateFreeAsyncWait& s) {
        m_thread = s.m_thread;
        m_timeout = s.m_timeout;
        m_success = s.m_success;
    }

    DelegateThread& m_thread;               // Target thread to invoke the delegate function
    bool m_success = false;			        // Set to true if async function succeeds
    std::chrono::milliseconds m_timeout;    // Time in mS to wait for async function to invoke
    Semaphore m_sema;				        // Semaphore to signal waiting thread
    bool m_sync = false;                    // Set true when synchronous invocation is required
    DelegateFreeAsyncWaitInvoke<RetType(Args...)> m_invoke;
};

template <class C, class R>
struct DelegateMemberAsyncWait; // Not defined

template <class TClass, class RetType, class... Args>
class DelegateMemberAsyncWait<TClass, RetType(Args...)> : public DelegateMember<TClass, RetType(Args...)>, public IDelegateInvoker {
public:
    typedef std::integral_constant<std::size_t, sizeof...(Args)> ArgCnt;
    typedef TClass* ObjectPtr;
    typedef RetType(TClass::* MemberFunc)(Args...);
    typedef RetType(TClass::* ConstMemberFunc)(Args...) const;
    using ClassType = DelegateMemberAsyncWait<TClass, RetType(Args...)>;
    using BaseType = DelegateMember<TClass, RetType(Args...)>;

    // Contructors take a class instance, member function, and delegate thread
    DelegateMemberAsyncWait(ObjectPtr object, MemberFunc func, DelegateThread& thread, std::chrono::milliseconds timeout) :
        BaseType(object, func), m_thread(thread), m_timeout(timeout) {
        Bind(object, func, thread);
    }
    DelegateMemberAsyncWait(ObjectPtr object, ConstMemberFunc func, DelegateThread& thread, std::chrono::milliseconds timeout) :
        BaseType(object, func), m_thread(thread), m_timeout(timeout) {
        Bind(object, func, thread);
    }
    DelegateMemberAsyncWait(const DelegateMemberAsyncWait& rhs) : BaseType(rhs), m_thread(rhs.m_thread), m_sync(false) {
        Swap(rhs);
    }
    DelegateMemberAsyncWait() = delete;

    virtual ClassType* Clone() const override {
        return new ClassType(*this);
    }

    /// Bind a member function to a delegate. 
    void Bind(ObjectPtr object, MemberFunc func, DelegateThread& thread) {
        m_thread = thread;
        BaseType::Bind(object, func);
    }

    /// Bind a const member function to a delegate. 
    void Bind(ObjectPtr object, ConstMemberFunc func, DelegateThread& thread) {
        m_thread = thread;
        BaseType::Bind(object, func);
    }

    virtual bool operator==(const DelegateBase& rhs) const override {
        auto derivedRhs = dynamic_cast<const ClassType*>(&rhs);
        return derivedRhs &&
            &m_thread == &derivedRhs->m_thread &&
            BaseType::operator==(rhs);
    }

    DelegateMemberAsyncWait& operator=(const DelegateMemberAsyncWait& rhs) {
        if (&rhs != this) {
            BaseType::operator=(rhs);
            Swap(rhs);
        }
        return *this;
    }

    /// Invoke delegate function asynchronously
    virtual RetType operator()(Args... args) override {
        if (m_sync)
            return BaseType::operator()(args...);
        else
        {
            // Create a clone instance of this delegate 
            auto delegate = std::shared_ptr<ClassType>(Clone());

            static_assert(ArgCnt::value <= 5, "Maximum arguments exceeded");
            if constexpr (ArgCnt::value == 0)
            {
                auto msg = std::make_shared<DelegateMsgBase>(delegate);
                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 1)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);

                using Param1 = ArgTypeOf<0, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                auto msg = std::make_shared<DelegateMsg1<Param1>>(delegate, heap_p1);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 2)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                auto msg = std::make_shared<DelegateMsg2<Param1, Param2>>(delegate, heap_p1, heap_p2);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 3)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                auto msg = std::make_shared<DelegateMsg3<Param1, Param2, Param3>>(delegate, heap_p1, heap_p2, heap_p3);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 4)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);
                decltype(auto) p4 = ArgValueOf<3>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;
                using Param4 = ArgTypeOf<3, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                decltype(auto) heap_p4 = DelegateParam<Param4>::New(p4);
                auto msg = std::make_shared<DelegateMsg4<Param1, Param2, Param3, Param4>>(delegate, heap_p1, heap_p2, heap_p3, heap_p4);

                m_thread.DispatchDelegate(msg);
            }
            else if constexpr (ArgCnt::value == 5)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);
                decltype(auto) p4 = ArgValueOf<3>(args...);
                decltype(auto) p5 = ArgValueOf<4>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;
                using Param4 = ArgTypeOf<3, Args...>;
                using Param5 = ArgTypeOf<4, Args...>;

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                decltype(auto) heap_p4 = DelegateParam<Param4>::New(p4);
                decltype(auto) heap_p5 = DelegateParam<Param5>::New(p5);
                auto msg = std::make_shared<DelegateMsg5<Param1, Param2, Param3, Param4, Param5>>(delegate, heap_p1, heap_p2, heap_p3, heap_p4, heap_p5);

                m_thread.DispatchDelegate(msg);
            }

            // Wait for target thread to execute the delegate target function
            if ((m_success = delegate->m_sema.Wait(m_timeout)))
                m_invoke = delegate->m_invoke;

            return m_invoke.GetRetVal();
        }
    }

    /// Invoke delegate function asynchronously
    auto AsyncInvoke(Args... args)
    {
        if constexpr (std::is_void<RetType>::value == true)
        {
            operator()(args...);
            return IsSuccess() ? std::optional<bool>(true) : std::optional<bool>();
        }
        else
        {
            auto retVal = operator()(args...);
            return IsSuccess() ? std::optional<RetType>(retVal) : std::optional<RetType>();
        }
    }

    /// Called by the target thread to invoke the delegate function 
    virtual void DelegateInvoke(std::shared_ptr<DelegateMsgBase> msg) override {
        m_sync = true;

        static_assert(ArgCnt::value <= 5, "Maximum arguments exceeded");
        if constexpr (ArgCnt::value == 0)
        {
            m_invoke(this);
        }
        else if constexpr (ArgCnt::value == 1)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg1<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            m_invoke(this, p1);
    }
        else if constexpr (ArgCnt::value == 2)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg2<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            m_invoke(this, p1, p2);
        }
        else if constexpr (ArgCnt::value == 3)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg3<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            auto p3 = delegateMsg->GetParam3();
            m_invoke(this, p1, p2, p3);
        }
        else if constexpr (ArgCnt::value == 4)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg4<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            auto p3 = delegateMsg->GetParam3();
            auto p4 = delegateMsg->GetParam4();
            m_invoke(this, p1, p2, p3, p4);
        }
        else if constexpr (ArgCnt::value == 5)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg5<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto p1 = delegateMsg->GetParam1();
            auto p2 = delegateMsg->GetParam2();
            auto p3 = delegateMsg->GetParam3();
            auto p4 = delegateMsg->GetParam4();
            auto p5 = delegateMsg->GetParam5();
            m_invoke(this, p1, p2, p3, p4, p5);
        }

        // Signal the waiting thread
        m_sema.Signal();
    }

    /// Returns true if asynchronous function successfully invoked on target thread
    bool IsSuccess() { return m_success; }

    /// Returns the async function return value
    RetType GetRetVal() { return m_invoke.GetRetVal(); }

private:
    void Swap(const DelegateMemberAsyncWait& s) {
        m_thread = s.m_thread;
        m_timeout = s.m_timeout;
        m_success = s.m_success;
    }

    DelegateThread& m_thread;	            // Target thread to invoke the delegate function
    bool m_success = false;					// Set to true if async function succeeds
    std::chrono::milliseconds m_timeout;    // Time in mS to wait for async function to invoke
    Semaphore m_sema;				        // Semaphore to signal waiting thread
    bool m_sync = false;                    // Set true when synchronous invocation is required
    DelegateMemberAsyncWaitInvoke<TClass, RetType(Args...)> m_invoke;
};

template <class TClass, class RetType, class... Args>
DelegateMemberAsyncWait<TClass, RetType(Args...)> MakeDelegate(TClass* object, RetType(TClass::* func)(Args... args), DelegateThread& thread, std::chrono::milliseconds timeout) {
    return DelegateMemberAsyncWait<TClass, RetType(Args...)>(object, func, thread, timeout);
}

template <class TClass, class RetType, class... Args>
DelegateMemberAsyncWait<TClass, RetType(Args...)> MakeDelegate(TClass* object, RetType(TClass::* func)(Args... args) const, DelegateThread& thread, std::chrono::milliseconds timeout) {
    return DelegateMemberAsyncWait<TClass, RetType(Args...)>(object, func, thread, timeout);
}

template <class RetType, class... Args>
DelegateFreeAsyncWait<RetType(Args...)> MakeDelegate(RetType(*func)(Args... args), DelegateThread& thread, std::chrono::milliseconds timeout) {
    return DelegateFreeAsyncWait<RetType(Args...)>(func, thread, timeout);
}

} 

#endif
