#ifndef _DELEGATE_SP_ASYNC_H
#define _DELEGATE_SP_ASYNC_H

// Delegate.h
// @see https://github.com/endurodave/AsyncMulticastDelegateCpp17
// David Lafreniere, Oct 2022.
//
// The DelegateMemberSpX delegate implemenations asynchronously bind and invoke class instance member functions. 
// The std::shared_ptr<TClass> is used in lieu of a raw TClass* pointer. 

#include "DelegateSp.h"
#include "IDelegateThread.h"
#include "DelegateInvoker.h"

namespace DelegateLib {

template <class C, class R>
struct DelegateMemberAsyncSp; // Not defined

/// @brief The DelegateMemberAsyncSp delegate implemenation asynchronously binds and 
/// and invokes class instance member functions. The std::shared_ptr<TClass> is used in 
/// lieu of a raw TClass* pointer. 
template <class TClass, class... Args>
class DelegateMemberAsyncSp<TClass, void(Args...)> : public DelegateMemberSp<TClass, void(Args...)>, public IDelegateInvoker {
public:
    typedef std::integral_constant<std::size_t, sizeof...(Args)> ArgCnt;
    typedef std::shared_ptr<TClass> ObjectPtr;
    typedef void (TClass::* MemberFunc)(Args...);
    typedef void (TClass::* ConstMemberFunc)(Args...) const;
    using ClassType = DelegateMemberAsyncSp<TClass, void(Args...)>;
    using BaseType = DelegateMemberSp<TClass, void(Args...)>;

    // Contructors take a class instance, member function, and callback thread
    DelegateMemberAsyncSp(ObjectPtr object, MemberFunc func, DelegateThread& thread) : BaseType(object, func), m_thread(thread) {
        Bind(object, func, thread);
    }
    DelegateMemberAsyncSp(ObjectPtr object, ConstMemberFunc func, DelegateThread& thread) : BaseType(object, func), m_thread(thread) {
        Bind(object, func, thread);
    }
    DelegateMemberAsyncSp() = delete;

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

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                auto msg = std::make_shared<DelegateMsg1<Param1>>(delegate, heap_p1);

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

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                auto msg = std::make_shared<DelegateMsg2<Param1, Param2>>(delegate, heap_p1, heap_p2);

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

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                auto msg = std::make_shared<DelegateMsg3<Param1, Param2, Param3>>(delegate, heap_p1, heap_p2, heap_p3);

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

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                decltype(auto) heap_p4 = DelegateParam<Param4>::New(p4);
                auto msg = std::make_shared<DelegateMsg4<Param1, Param2, Param3, Param4>>(delegate, heap_p1, heap_p2, heap_p3, heap_p4);

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

                decltype(auto) heap_p1 = DelegateParam<Param1>::New(p1);
                decltype(auto) heap_p2 = DelegateParam<Param2>::New(p2);
                decltype(auto) heap_p3 = DelegateParam<Param3>::New(p3);
                decltype(auto) heap_p4 = DelegateParam<Param4>::New(p4);
                decltype(auto) heap_p5 = DelegateParam<Param5>::New(p5);
                auto msg = std::make_shared<DelegateMsg5<Param1, Param2, Param3, Param4, Param5>>(delegate, heap_p1, heap_p2, heap_p3, heap_p4, heap_p5);

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
    virtual void DelegateInvoke(std::shared_ptr<DelegateMsgBase> msg) override {
        m_sync = true;

        static_assert(ArgCnt::value <= 5, "Maximum arguments exceeded");
        if constexpr (ArgCnt::value == 0)
        {
            BaseType::operator()();
        }
        else if constexpr (ArgCnt::value == 1)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg1<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            BaseType::operator()(param1);
    }
        else if constexpr (ArgCnt::value == 2)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg2<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            BaseType::operator()(param1, param2);
        }
        else if constexpr (ArgCnt::value == 3)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg3<Args...>>(msg);
            if (delegateMsg == nullptr)
                throw std::invalid_argument("Invalid std::dynamic_pointer_cast");
            auto param1 = delegateMsg->GetParam1();
            auto param2 = delegateMsg->GetParam2();
            auto param3 = delegateMsg->GetParam3();
            BaseType::operator()(param1, param2, param3);
        }
        else if constexpr (ArgCnt::value == 4)
        {
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg4<Args...>>(msg);
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
            auto delegateMsg = std::dynamic_pointer_cast<DelegateMsg5<Args...>>(msg);
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
DelegateMemberAsyncSp<TClass, void(Args...)> MakeDelegate(std::shared_ptr<TClass> object, void(TClass::* func)(Args... args), DelegateThread& thread) {
    return DelegateMemberAsyncSp<TClass, void(Args...)>(object, func, thread);
}

template <class TClass, class... Args>
DelegateMemberAsyncSp<TClass, void(Args...)> MakeDelegate(std::shared_ptr<TClass> object, void(TClass::* func)(Args... args) const, DelegateThread& thread) {
    return DelegateMemberAsyncSp<TClass, void(Args...)>(object, func, thread);
}

}

#endif
