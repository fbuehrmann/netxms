/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: queue.cpp
**
**/

#include "libnetxms.h"
#include <nxqueue.h>

/**
 * Queue constructor
 */
Queue::Queue(size_t initialSize, size_t bufferIncrement, bool owner)
{
   m_initialSize = initialSize;
   m_bufferSize = initialSize;
   m_bufferIncrement = bufferIncrement;
   m_owner = owner;
	commonInit();
}

/**
 * Default queue constructor
 */
Queue::Queue(bool owner)
{
   m_initialSize = 256;
   m_bufferSize = 256;
   m_bufferIncrement = 32;
   m_owner = owner;
	commonInit();
}

/**
 * Common initialization (used by all constructors)
 */
void Queue::commonInit()
{
   m_mutexQueueAccess = MutexCreate();
   m_condWakeup = ConditionCreate(FALSE);
   m_numElements = 0;
   m_first = 0;
   m_last = 0;
   m_elements = MemAllocArray<void*>(m_bufferSize);
	m_shutdownFlag = false;
	m_destructor = MemFree;
}

/**
 * Destructor
 */
Queue::~Queue()
{
   if (m_owner)
   {
      for(size_t i = 0, pos = m_first; i < m_numElements; i++)
      {
         if (m_elements[pos] != INVALID_POINTER_VALUE)
            m_destructor(m_elements[pos]);
         pos++;
         if (pos == m_bufferSize)
            pos = 0;
      }
   }
   MutexDestroy(m_mutexQueueAccess);
   ConditionDestroy(m_condWakeup);
   MemFree(m_elements);
}

/**
 * Put new element into queue
 */
void Queue::put(void *pElement)
{
   lock();
   if (m_numElements == m_bufferSize)
   {
      // Extend buffer
      m_bufferSize += m_bufferIncrement;
      m_elements = MemReallocArray(m_elements, m_bufferSize);
      
      // Move free space
      memmove(&m_elements[m_first + m_bufferIncrement], &m_elements[m_first],
              sizeof(void *) * (m_bufferSize - m_first - m_bufferIncrement));
      m_first += m_bufferIncrement;
   }
   m_elements[m_last++] = pElement;
   if (m_last == m_bufferSize)
      m_last = 0;
   m_numElements++;
   ConditionSet(m_condWakeup);
   unlock();
}

/**
 * Insert new element into the beginning of a queue
 */
void Queue::insert(void *pElement)
{
   lock();
   if (m_numElements == m_bufferSize)
   {
      // Extend buffer
      m_bufferSize += m_bufferIncrement;
      m_elements = MemReallocArray(m_elements, m_bufferSize);
      
      // Move free space
      memmove(&m_elements[m_first + m_bufferIncrement], &m_elements[m_first],
              sizeof(void *) * (m_bufferSize - m_first - m_bufferIncrement));
      m_first += m_bufferIncrement;
   }
   if (m_first == 0)
      m_first = m_bufferSize;
   m_elements[--m_first] = pElement;
   m_numElements++;
   ConditionSet(m_condWakeup);
   unlock();
}

/**
 * Get object from queue. Return NULL if queue is empty
 */
void *Queue::get()
{
   void *pElement = NULL;

   lock();
	if (m_shutdownFlag)
	{
		pElement = INVALID_POINTER_VALUE;
	}
	else
   {
		while((m_numElements > 0) && (pElement == NULL))
		{
			pElement = m_elements[m_first++];
			if (m_first == m_bufferSize)
				m_first = 0;
			m_numElements--;
		}
      shrink();
   }
   unlock();
   return pElement;
}

/**
 * Get object from queue or block with timeout if queue if empty
 */
void *Queue::getOrBlock(UINT32 timeout)
{
   void *pElement = get();
   if (pElement != NULL)
   {
      return pElement;
   }

   do
   {
      if (!ConditionWait(m_condWakeup, timeout))
         break;
      pElement = get();
   } while(pElement == NULL);
   return pElement;
}

/**
 * Clear queue
 */
void Queue::clear()
{
   lock();
   if (m_owner)
   {
      for(size_t i = 0, pos = m_first; i < m_numElements; i++)
      {
         if (m_elements[pos] != INVALID_POINTER_VALUE)
            m_destructor(m_elements[pos]);
         pos++;
         if (pos == m_bufferSize)
            pos = 0;
      }
   }
   m_numElements = 0;
   m_first = 0;
   m_last = 0;
   shrink();
   unlock();
}

/**
 * Set shutdown flag
 * When this flag is set, Get() always return INVALID_POINTER_VALUE
 */
void Queue::setShutdownMode()
{
	lock();
	m_shutdownFlag = true;
	ConditionSet(m_condWakeup);
	unlock();
}

/**
 * Find element in queue using given key and comparator
 * Returns pointer to element or NULL if element was not found.
 * Element remains in the queue
 */
void *Queue::find(const void *key, QueueComparator comparator)
{
	void *element = NULL;

	lock();
	for(size_t i = 0, pos = m_first; i < m_numElements; i++)
	{
		if ((m_elements[pos] != NULL) && (m_elements[pos] != INVALID_POINTER_VALUE) && comparator(key, m_elements[pos]))
		{
			element = m_elements[pos];
			break;
		}
		pos++;
		if (pos == m_bufferSize)
			pos = 0;
	}
	unlock();
	return element;
}

/**
 * Find element in queue using given key and comparator and remove it.
 * Returns true if element was removed.
 */
bool Queue::remove(const void *key, QueueComparator comparator)
{
	bool success = false;

	lock();
	for(size_t i = 0, pos = m_first; i < m_numElements; i++)
	{
		if ((m_elements[pos] != NULL) && comparator(key, m_elements[pos]))
		{
		   if (m_owner && (m_elements[pos] != INVALID_POINTER_VALUE))
		      m_destructor(m_elements[pos]);
			m_elements[pos] = NULL;
			success = true;
			break;
		}
		pos++;
		if (pos == m_bufferSize)
			pos = 0;
	}
	unlock();
	return success;
}

/**
 * Enumerate queue elements
 */
void Queue::forEach(QueueEnumerationCallback callback, void *context)
{
   lock();
   for(size_t i = 0, pos = m_first; i < m_numElements; i++)
   {
      if ((m_elements[pos] != NULL) && (m_elements[pos] != INVALID_POINTER_VALUE))
      {
         if (callback(m_elements[pos], context) == _STOP)
            break;
      }
      pos++;
      if (pos == m_bufferSize)
         pos = 0;
   }
   unlock();
}

/**
 * Shrink queue if possible
 */
void Queue::shrink()
{
   if ((m_bufferSize == m_initialSize) || (m_numElements > m_initialSize / 2) || ((m_numElements > 0) && (m_last < m_first)))
      return;

   if ((m_numElements > 0) && (m_first > 0))
   {
      memmove(&m_elements[0], &m_elements[m_first], sizeof(void *) * m_numElements);
      m_last -= m_first;
      m_first = 0;
   }
   m_bufferSize = m_initialSize;
   m_elements = MemReallocArray(m_elements, m_bufferSize);
}
