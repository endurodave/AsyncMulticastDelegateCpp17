#ifndef _DELEGATE_ASYNC_H
#define _DELEGATE_ASYNC_H

// Delegate.h
// @see https://github.com/endurodave/AsyncMulticastDelegateCpp17
// David Lafreniere, Oct 2022.

#include "Delegate.h"
#include "IDelegateThread.h"
#include "DelegateInvoker.h"
#include <memory>
#include <type_traits>
#include <tuple>
#ifdef USE_XALLOCATOR
	#include <new>
#endif

namespace DelegateLib {

// Get the type of the Nth position within a template parameter pack
template<size_t N, class... Args> using ArgTypeOf =
    typename std::tuple_element<N, std::tuple<Args...>>::type;

// Get the value of the Nth position within a template parameter pack
template <size_t N, class... Args>
decltype(auto) ArgValueOf(Args&&... ts) {
    return std::get<N>(std::forward_as_tuple(ts...));
}

// std::shared_ptr reference arguments are not allowed with asynchronous delegates as the behavior is 
// undefined. In other words:
// void MyFunc(std::shared_ptr<T> data)		// Ok!
// void MyFunc(std::shared_ptr<T>& data)	// Error if DelegateAsync or DelegateSpAsync target!
template<class T>
struct is_shared_ptr : std::false_type {};

template<class T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template<class T>
struct is_shared_ptr<std::shared_ptr<T>&> : std::true_type {};

template<class T>
struct is_shared_ptr<const std::shared_ptr<T>&> : std::true_type {};

template<class T>
struct is_shared_ptr<std::shared_ptr<T>*> : std::true_type {};

template<class T>
struct is_shared_ptr<const std::shared_ptr<T>*> : std::true_type {};

template <class R>
struct DelegateFreeAsync; // Not defined

template <class... Args> 
class DelegateFreeAsync<void(Args...)> : public DelegateFree<void(Args...)>, public IDelegateInvoker {
public:
    typedef std::integral_constant<std::size_t, sizeof...(Args)> ArgCnt;
    typedef void(*FreeFunc)(Args...);
    using ClassType = DelegateFreeAsync<void(Args...)>;
    using BaseType = DelegateFree<void(Args...)>;

    DelegateFreeAsync(FreeFunc func, DelegateThread& thread) : BaseType(func), m_thread(thread) { Bind(func, thread); }
    DelegateFreeAsync() = delete;

    /// Bind a free function to the delegate.
    void Bind(FreeFunc func, DelegateThread& thread) {
        m_thread = thread;
        BaseType::Bind(func);
    }

    virtual ClassType* Clone() const override {
        return new ClassType(*this);
    }

    virtual bool operator==(const DelegateBase& rhs) const override {
        auto derivedRhs = dynamic_cast<const ClassType*>(&rhs);
        return derivedRhs &&
            &m_thread == &derivedRhs->m_thread &&
            BaseType::operator == (rhs);
    }

    // Invoke delegate function asynchronously
    virtual void operator()(Args... args) override {
        if (m_sync)
            BaseType::operator()(args...);
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

                auto msg = std::make_shared<DelegateMsgHeapParam1<Param1>>(delegate, p1);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value))),
                    "std::shared_ptr reference argument not allowed");
            }
            else if constexpr (ArgCnt::value == 2)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;

                auto msg = std::make_shared<DelegateMsgHeapParam2<Param1, Param2>>(delegate, p1, p2);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value))),
                    "std::shared_ptr reference argument not allowed");
            }
            else if constexpr (ArgCnt::value == 3)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;

                auto msg = std::make_shared<DelegateMsgHeapParam3<Param1, Param2, Param3>>(delegate, p1, p2, p3);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value)) ||
                    (is_shared_ptr<Param3>::value && (std::is_lvalue_reference<Param3>::value || std::is_pointer<Param3>::value))),
                    "std::shared_ptr reference argument not allowed");
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

                auto msg = std::make_shared<DelegateMsgHeapParam4<Param1, Param2, Param3, Param4>>(delegate, p1, p2, p3, p4);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value)) ||
                    (is_shared_ptr<Param3>::value && (std::is_lvalue_reference<Param3>::value || std::is_pointer<Param3>::value)) ||
                    (is_shared_ptr<Param4>::value && (std::is_lvalue_reference<Param4>::value || std::is_pointer<Param4>::value))),
                    "std::shared_ptr reference argument not allowed");
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

                auto msg = std::make_shared<DelegateMsgHeapParam5<Param1, Param2, Param3, Param4, Param5>>(delegate, p1, p2, p3, p4, p5);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value)) ||
                    (is_shared_ptr<Param3>::value && (std::is_lvalue_reference<Param3>::value || std::is_pointer<Param3>::value)) ||
                    (is_shared_ptr<Param4>::value && (std::is_lvalue_reference<Param4>::value || std::is_pointer<Param4>::value)) ||
                    (is_shared_ptr<Param5>::value && (std::is_lvalue_reference<Param5>::value || std::is_pointer<Param5>::value))),
                    "std::shared_ptr reference argument not allowed");
            }
        }
    }

    // Called to invoke the delegate function on the target thread of control
    virtual void DelegateInvoke(std::shared_ptr<DelegateMsgBase> msg) {
        m_sync = true;

        static_assert(ArgCnt::value <= 5, "Maximum arguments exceeded");
        if constexpr (ArgCnt::value == 0)
        {
            BaseType::operator()();
        }
        else if constexpr (ArgCnt::value == 1)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam1<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            BaseType::operator()(param1);
        }
        else if constexpr (ArgCnt::value == 2)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam2<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            BaseType::operator()(param1, param2);
        }
        else if constexpr (ArgCnt::value == 3)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam3<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            auto param3 = delegateMsg->GetParam3();
            BaseType::operator()(param1, param2, param3);
        }
        else if constexpr (ArgCnt::value == 4)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam4<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            auto param3 = delegateMsg->GetParam3();
            auto param4 = delegateMsg->GetParam4();
            BaseType::operator()(param1, param2, param3, param4);
        }
        else if constexpr (ArgCnt::value == 5)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam5<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            auto param3 = delegateMsg->GetParam3();
            auto param4 = delegateMsg->GetParam4();
            auto param5 = delegateMsg->GetParam5();
            BaseType::operator()(param1, param2, param3, param4, param5);
        }
    }

private:
    DelegateThread& m_thread; 
    bool m_sync = false;
};

template <class C, class R>
struct DelegateMemberAsync; // Not defined

template <class TClass, class... Args>
class DelegateMemberAsync<TClass, void(Args...)> : public DelegateMember<TClass, void(Args...)>, public IDelegateInvoker {
public:
    typedef std::integral_constant<std::size_t, sizeof...(Args)> ArgCnt;
    typedef TClass* ObjectPtr;
    typedef void (TClass::*MemberFunc)(Args...);
    typedef void (TClass::*ConstMemberFunc)(Args...) const;
    using ClassType = DelegateMemberAsync<TClass, void(Args...)>;
    using BaseType = DelegateMember<TClass, void(Args...)>;

    // Contructors take a class instance, member function, and callback thread
    DelegateMemberAsync(ObjectPtr object, MemberFunc func, DelegateThread& thread) : BaseType(object, func), m_thread(thread)
        { Bind(object, func, thread); }
    DelegateMemberAsync(ObjectPtr object, ConstMemberFunc func, DelegateThread& thread) : BaseType(object, func), m_thread(thread)
        { Bind(object, func, thread); }
    DelegateMemberAsync() = delete;

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

    virtual ClassType* Clone() const override {
        return new ClassType(*this);
    }

    virtual bool operator==(const DelegateBase& rhs) const override {
        auto derivedRhs = dynamic_cast<const ClassType*>(&rhs);
        return derivedRhs &&
            &m_thread == &derivedRhs->m_thread &&
            BaseType::operator == (rhs);
    }

    /// Invoke delegate function asynchronously
    virtual void operator()(Args... args) override {
        if (m_sync)
            BaseType::operator()(args...);
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

                auto msg = std::make_shared<DelegateMsgHeapParam1<Param1>>(delegate, p1);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value))),
                    "std::shared_ptr reference argument not allowed");
            }
            else if constexpr (ArgCnt::value == 2)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;

                auto msg = std::make_shared<DelegateMsgHeapParam2<Param1, Param2>>(delegate, p1, p2);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value))),
                    "std::shared_ptr reference argument not allowed");
            }
            else if constexpr (ArgCnt::value == 3)
            {
                decltype(auto) p1 = ArgValueOf<0>(args...);
                decltype(auto) p2 = ArgValueOf<1>(args...);
                decltype(auto) p3 = ArgValueOf<2>(args...);

                using Param1 = ArgTypeOf<0, Args...>;
                using Param2 = ArgTypeOf<1, Args...>;
                using Param3 = ArgTypeOf<2, Args...>;

                auto msg = std::make_shared<DelegateMsgHeapParam3<Param1, Param2, Param3>>(delegate, p1, p2, p3);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value)) ||
                    (is_shared_ptr<Param3>::value && (std::is_lvalue_reference<Param3>::value || std::is_pointer<Param3>::value))),
                    "std::shared_ptr reference argument not allowed");
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

                auto msg = std::make_shared<DelegateMsgHeapParam4<Param1, Param2, Param3, Param4>>(delegate, p1, p2, p3, p4);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value)) ||
                    (is_shared_ptr<Param3>::value && (std::is_lvalue_reference<Param3>::value || std::is_pointer<Param3>::value)) ||
                    (is_shared_ptr<Param4>::value && (std::is_lvalue_reference<Param4>::value || std::is_pointer<Param4>::value))),
                    "std::shared_ptr reference argument not allowed");
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

                auto msg = std::make_shared<DelegateMsgHeapParam5<Param1, Param2, Param3, Param4, Param5>>(delegate, p1, p2, p3, p4, p5);

                m_thread.DispatchDelegate(msg);

                static_assert(!(
                    (is_shared_ptr<Param1>::value && (std::is_lvalue_reference<Param1>::value || std::is_pointer<Param1>::value)) ||
                    (is_shared_ptr<Param2>::value && (std::is_lvalue_reference<Param2>::value || std::is_pointer<Param2>::value)) ||
                    (is_shared_ptr<Param3>::value && (std::is_lvalue_reference<Param3>::value || std::is_pointer<Param3>::value)) ||
                    (is_shared_ptr<Param4>::value && (std::is_lvalue_reference<Param4>::value || std::is_pointer<Param4>::value)) ||
                    (is_shared_ptr<Param5>::value && (std::is_lvalue_reference<Param5>::value || std::is_pointer<Param5>::value))),
                    "std::shared_ptr reference argument not allowed");
            }
        }
    }

    /// Called by the target thread to invoke the delegate function 
    virtual void DelegateInvoke(std::shared_ptr<DelegateMsgBase> msg) {
        m_sync = true;

        static_assert(ArgCnt::value <= 5, "Maximum arguments exceeded");
        if constexpr (ArgCnt::value == 0)
        {
            BaseType::operator()();
        }
        else if constexpr (ArgCnt::value == 1)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam1<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            BaseType::operator()(param1);
    }
        else if constexpr (ArgCnt::value == 2)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam2<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            BaseType::operator()(param1, param2);
        }
        else if constexpr (ArgCnt::value == 3)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam3<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            auto param3 = delegateMsg->GetParam3();
            BaseType::operator()(param1, param2, param3);
        }
        else if constexpr (ArgCnt::value == 4)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam4<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            auto param3 = delegateMsg->GetParam3();
            auto param4 = delegateMsg->GetParam4();
            BaseType::operator()(param1, param2, param3, param4);
        }
        else if constexpr (ArgCnt::value == 5)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsgHeapParam5<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            auto param3 = delegateMsg->GetParam3();
            auto param4 = delegateMsg->GetParam4();
            auto param5 = delegateMsg->GetParam5();
            BaseType::operator()(param1, param2, param3, param4, param5);
        }
    }

private:
    /// Target thread to invoke the delegate function
    DelegateThread& m_thread;
    bool m_sync = false;
};

template <class TClass, class... Args>
DelegateMemberAsync<TClass, void(Args...)> MakeDelegate(TClass* object, void(TClass::*func)(Args... args), DelegateThread& thread) {
    return DelegateMemberAsync<TClass, void(Args...)>(object, func, thread);
}

template <class TClass, class... Args>
DelegateMemberAsync<TClass, void(Args...)> MakeDelegate(TClass* object, void(TClass::*func)(Args... args) const, DelegateThread& thread) {
    return DelegateMemberAsync<TClass, void(Args...)>(object, func, thread);
}

template <class... Args>
DelegateFreeAsync<void(Args...)> MakeDelegate(void(*func)(Args... args), DelegateThread& thread) {
    return DelegateFreeAsync<void(Args...)>(func, thread);
}

}

#endif
