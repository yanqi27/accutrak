
#ifndef _EVENT_H
#define _EVENT_H
/************************************************************************
** FILE NAME..... Event.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Primitive of specialized event object.
**
** NOTES............  
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS..... 
**
** LIMITATIONS...... 
** 
** DEVIATIONS....... 
** 
** RETURN VALUES.... 0  - successful
**                   !0 - error
** 
** AUTHOR(S)........ Michael Q Yan
** 
** CHANGES:
** 
************************************************************************/
class Event
{
public:

	Event() : mTotalThreads(0), mWaitingThreads(0)
	{
		if(::pthread_cond_init(&mConditionVariable, NULL))
		{
			fprintf(stderr, "Failed to pthread_cond_init\n");
		}
		if(::pthread_mutex_init(&mMutex, NULL))
		{
			fprintf(stderr, "Failed to pthread_mutex_init\n");
		}
	}

	~Event()
	{
		if(::pthread_cond_destroy(&mConditionVariable))
		{
			fprintf(stderr, "Failed to pthread_cond_destroy\n");
		}
		if(::pthread_mutex_destroy(&mMutex))
		{
			fprintf(stderr, "Failed to pthread_mutex_destroy\n");
		}
	}

	void Reset(int iNumThreads) 
	{ 
		mTotalThreads = iNumThreads;
		mWaitingThreads = 0;
	}

	void Set()
	{
		// wait until all threads are in waiting status
		while(1)
		{
			::pthread_mutex_lock(&mMutex);
			if(mWaitingThreads == mTotalThreads)
			{
				break;
			}
			// yield ??
			::pthread_mutex_unlock(&mMutex);
		}

		if(::pthread_cond_broadcast(&mConditionVariable))
		{
			fprintf(stderr, "Failed to pthread_cond_broadcast\n");
		}

		// Auto reset.
		mWaitingThreads = 0;
		::pthread_mutex_unlock(&mMutex);
	}

	void WaitForever()
	{
		::pthread_mutex_lock(&mMutex);

		mWaitingThreads++;

		if(::pthread_cond_wait(&mConditionVariable, &mMutex) )
		{
			fprintf(stderr, "Failed to pthread_cond_wait\n");
		}

		::pthread_mutex_unlock(&mMutex);
	}

private:
	int    mTotalThreads;
	int    mWaitingThreads;
#ifdef WIN32
	HANDLE mEventHandle;
#else
	pthread_mutex_t mMutex;
	pthread_cond_t  mConditionVariable;
#endif
};


#endif // _EVENT_H
