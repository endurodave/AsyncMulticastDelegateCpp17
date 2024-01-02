#ifndef _DELEGATE_PARAM_H
#define _DELEGATE_PARAM_H


namespace DelegateLib
{

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
#ifdef USE_XALLOCATOR
		void* mem = xmalloc(sizeof(*param));
		Param* newParam = new (mem) Param(*param);
#else
		Param* newParam = new Param(*param);
#endif
		return newParam;
	}

	static void Delete(Param* param) {
#ifdef USE_XALLOCATOR
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
#ifdef USE_XALLOCATOR
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
#ifdef USE_XALLOCATOR
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
#ifdef USE_XALLOCATOR
		void* mem = xmalloc(sizeof(param));
		Param* newParam = new (mem) Param(param);
#else
		Param* newParam = new Param(param);
#endif
		return *newParam;
	}

	static void Delete(Param& param) {
#ifdef USE_XALLOCATOR
		(&param)->~Param();
		xfree((void*)(&param));
#else
		delete &param;
#endif
	}
};

}

#endif
