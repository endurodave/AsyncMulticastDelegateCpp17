#ifndef _THREAD_MSG_H
#define _THREAD_MSG_H

#ifdef USE_XALLOCATOR
	#include "xallocator.h"
#endif

/// @brief A class to hold a platform-specific thread messsage that will be passed 
/// through the OS message queue. 
class ThreadMsg
{
#ifdef USE_XALLOCATOR
	XALLOCATOR
#endif
public:
	/// Constructor
	/// @param[in] id - a unique identifier for the thread messsage
	/// @param[in] data - a pointer to the messsage data to be typecast
	///		by the receiving task based on the id value. 
	/// @pre The data pointer argument *must* be created on the heap.
	/// @port The destination thread will delete the heap allocated data once the 
	///		callback is complete.  
	ThreadMsg(INT id, std::shared_ptr<DelegateLib::DelegateMsgBase> data) :
		m_id(id), 
		m_data(data)
	{
	}

    INT GetId() const { return m_id; }
    std::shared_ptr<DelegateLib::DelegateMsgBase> GetData() { return m_data; }

private:
    INT m_id;
    std::shared_ptr<DelegateLib::DelegateMsgBase> m_data;
};

#endif
