#ifndef _DELEGATE_ASYNC_H
#define _DELEGATE_ASYNC_H

// Delegate.h
// @see https://www.codeproject.com/Articles/1160934/Asynchronous-Multicast-Delegates-in-Cplusplus
// David Lafreniere, Oct 2022.

#include "Delegate.h"
#include "IDelegateThread.h"
#include "DelegateInvoker.h"
#include <memory>
#include <type_traits>
#include <tuple>
#if USE_XALLOCATOR
	#include <new>
#endif

namespace DelegateLib {

template<size_t N, class... Args> using ArgTypeOf =
    typename std::tuple_element<N, std::tuple<Args...>>::type;

template <size_t N, class... Args>
decltype(auto) ArgValueOf(Args&&... ts) {
    return std::get<N>(std::forward_as_tuple(ts...));
}

// WORKS!
//template <size_t V, typename... T>
//decltype(auto) ArgValueOf(T&&... Args) noexcept {
    //return std::get<V>(std::forward_as_tuple(std::forward<T>(Args)...));
//}


// WORKS!
//template <size_t V, typename... T>
//std::tuple_element_t<V, std::tuple<T...>> ArgValueOf(T&&... Args) noexcept {
    //return std::get<V>(std::forward_as_tuple(std::forward<T>(Args)...));
//}

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

/// @brief Implements a new/delete for pass by value parameter values. Doesn't 
/// actually create memory as pass by value already has a full copy.
template <typename Param>
class DelegateParam
{
public:
	static Param New(Param param) {	return param; }
	static void Delete(Param param) { }
};

/// @brief Implement new/delete for pointer parameter values. If USE_ALLOCATOR is
/// defined, get memory from the fixed block allocator and not global heap.
template <typename Param>
class DelegateParam<Param *>
{
public:
	static Param* New(Param* param)	{
#if USE_XALLOCATOR
		void* mem = xmalloc(sizeof(*param));
		Param* newParam = new (mem) Param(*param);
#else
		Param* newParam = new Param(*param);
#endif
		return newParam;
	}

	static void Delete(Param* param) {
#if USE_XALLOCATOR
		param->~Param();
		xfree((void*)param);
#else
		delete param;
#endif
	}
};

/// @brief Implement new/delete for pointer to pointer parameter values. 
template <typename Param>
class DelegateParam<Param **>
{
public:
	static Param** New(Param** param) {
#if USE_XALLOCATOR
		void* mem = xmalloc(sizeof(*param));
		Param** newParam = new (mem) Param*();

		void* mem2 = xmalloc(sizeof(**param));
		*newParam = new (mem2) Param(**param);
#else
		Param** newParam = new Param*();
		*newParam = new Param(**param);
#endif
		return newParam;
	}

	static void Delete(Param** param) {
#if USE_XALLOCATOR
		(*param)->~Param();
		xfree((void*)(*param));

		xfree((void*)(param));
#else
		delete *param;
		delete param;
#endif
	}
};

/// @brief Implement new/delete for reference parameter values. 
template <typename Param>
class DelegateParam<Param &>
{
public:
	static Param& New(Param& param)	{
#if USE_XALLOCATOR
		void* mem = xmalloc(sizeof(param));
		Param* newParam = new (mem) Param(param);
#else
		Param* newParam = new Param(param);
#endif
		return *newParam;
	}

	static void Delete(Param& param) {
#if USE_XALLOCATOR
		(&param)->~Param();
		xfree((void*)(&param));
#else
		delete &param;
#endif
	}
};

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
