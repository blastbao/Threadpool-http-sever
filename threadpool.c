// Yoad Shiran - 302978713

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "threadpool.h"
/*----------------PDF-----------------
create_threadpool should:
1. Check the legacy of the parameter.
2. Create threadpool structure and initialize it:
a. num_thread = given parameter
b. qsize=0
c. threads = pointer to <num_thread> threads
d. qhead = qtail = NULL
e. Init lock and condition variables.
f.
shutdown = dont_accept = 0
g. Create the threads with do_work as execution function and the pool as an
argument.
--------------------------------------*/
threadpool* create_threadpool(int num_threads_in_pool)
{

	if ((num_threads_in_pool < 1) || (num_threads_in_pool > MAXT_IN_POOL))
    {return NULL;}
	threadpool *pool = (threadpool*)calloc(1,sizeof(threadpool));
	if (pool == NULL)
	{
		perror("Error with threadpool\n");
		return NULL;
	}
	int count = num_threads_in_pool;
	// pthread_t* thread =


	pool->num_threads = num_threads_in_pool; // a
	pool->qsize = 0;  // b
	pool->threads = (pthread_t*)calloc(num_threads_in_pool,sizeof(pthread_t));;  // c
	if (pool->threads == NULL)
	{
		perror("Error with (thread_t* thread)\n");
		return NULL;
	}
	// while(count>0)
	// {
	// 	pool->threads[count] = NULL;
	// 	count --;
	// }
	pool->qhead = NULL; //  d
	pool->qtail = NULL; //  d
	if (pthread_mutex_init(&(pool->qlock), NULL) != 0) // e
		{perror ("pthread_mutex_init");
		return NULL;}
	if (pthread_cond_init(&(pool->q_not_empty), NULL) != 0)
		{perror ("pthread_cond_init[0]");
		return NULL;}
	if (pthread_cond_init(&(pool->q_empty), NULL) != 0)
		{perror ("pthread_cond_init[1]");
	return NULL;}
	pool->shutdown = 0; 	  /// f
	pool->dont_accept = 0;    /// f

	for (count = 0; count < num_threads_in_pool; count++)
	{ // g

		if (pthread_create(&(pool->threads[count]), NULL, do_work, pool) != 0)
		{
			perror ("Prob - Create thread");
			return NULL;
		}
	}

	return (threadpool*)pool;
}

/*-------------------PDF-----------------
1. If destruction process has begun, exit thread
2. If the queue is empty, wait (no job to make)
3. Check again destruction flag.
4. Take the first element from the queue (*work_t)
5. If the queue becomes empty and destruction process wait to begin, signal
destruction process.
6. Call the thread routine.
 ----------------------------------------*/
void* do_work(void* p)
{

	if(p == NULL)
	{
			perror("arguments fail\n");
			return;
	}

	threadpool *pool = (threadpool*) p;
	while(1)
	{
		//1
		if (pthread_mutex_lock (&pool->qlock) != 0)
		{
			perror ("error with pthread_mutex_lock in do work");
			return;
		}

		if (pool->shutdown==1) // 1
		{
			if (pthread_mutex_unlock (&pool->qlock) != 0)
			{
				perror ("error with pthread_mutex_unlock in do work");
			}

			return NULL;
		}
		while(pool->qsize==0)
		{
			//destructionHasBegun(pool);
			//2
			if (pthread_cond_wait (&pool->q_empty, &pool->qlock) != 0)
			{
				perror ("error with pthread_cond_wait in do work");
				exit(EXIT_FAILURE);
			}

			//3
			if (pool->shutdown==1) // 1
			{
				if (pthread_mutex_unlock (&pool->qlock) != 0)
				{
					perror ("error with pthread_mutex_unlock in do work");
				}
				return NULL;
			}
		}
   		work_t* dowork = pool->qhead;
		void *in = (void*)(dowork->arg);
		if (pool->qsize == 0)
		{
			return;
		}
		//4
		if (pool->qsize == 1)
		{
	    	pool->qtail = NULL;
	    	pool->qhead = NULL;
	    }
	    else
	    {
	    	pool->qhead = (pool->qhead)->next;
	    }

	    pool->qsize--;
		//5
		if (pool->qsize==0 && pool->shutdown==0)
		{
     		if (pthread_cond_signal(&pool->q_not_empty) != 0)
     		{
     			perror("pthread_cond_signal error in do_work\n");
				return NULL;
     		}

   		}

   		//4 by header
   		if (pthread_mutex_unlock (&pool->qlock) != 0)
   		{
   			perror("pthread_mutex_unlock error in do_work\n");
			return NULL;
   		}

		//6 by PDF or 5 from header
		if (dowork!=NULL)
		{
	    	(dowork->routine)(in);
		}
		free(dowork);

	}

}


/*-------------------PDF-----------------------
1. Create work_t structure and init it with the routine and argument.
2. If destroy function has begun, don't accept new item to the queue
3. Add item to the queue
------------------------------------------------*/
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg)
{
	//2 if destroy has begun dont accept new item to the queue
	if (from_me->dont_accept == 1)
	{
		printf("dont accept in dispatch\n");
		// if (pthread_mutex_unlock(&(from_me->qlock)) != 0)
		// {
		// 	perror ("error in dispatch pthread_mutex_unlock");
		// 	exit(EXIT_FAILURE);
		// }
		return;
	}
	if (from_me == NULL || dispatch_to_here == NULL )
	{
		perror("Error with dispatch\n");
		return;
	}
	//1. create work_t strct and init it with routine and arg
	work_t* work = NULL;
	int begun = 0;
	work = (work_t*)calloc(1,sizeof(work_t));
	if (work == NULL)
	{
		perror("Error with create work (dispatch)\n");
		return;
	}
	work->routine = dispatch_to_here;
	work->arg = arg;
	work->next = NULL;
		 //* 1. lock mutex (by threadpool.h)
	if (pthread_mutex_lock(&(from_me->qlock)) != 0)
	{
		perror ("error in dispatch pthread_mutex_lock");
		return;
	}
	// 2-3  (by threadpool.h) AND 3 by pdf
	if (from_me->dont_accept == 1)
	{
		if (pthread_mutex_unlock(&(from_me->qlock)) != 0)
		{
			free(work);
			perror ("error in dispatch pthread_mutex_unlock");
			return;
		}
		free(work);
		return;
	}
	if (from_me->qhead == NULL)
	{
		from_me->qhead = work;
		from_me->qtail = work;
	}
	else
	{
		from_me->qtail->next = work;
		from_me->qtail = from_me->qtail->next;
	}
	from_me->qsize++;
	//4 (by threadpool.h)
	if (pthread_cond_signal(&from_me->q_empty) != 0)
	{
		perror("Error with pthread_cond_signal in dispatch\n");
		free(work);
		return;
	}

	if (pthread_mutex_unlock(&from_me->qlock)!=0)
	{
		perror("Error with mutex_unloc in dispatch\n");
		free(work);
		return;
	}
	//5 (by threadpool.h)
	//(work->routine) (work->arg);
	// free(work);

}

/* --------------PDF---------------------
destroy_threadpool
1. Set don’t_accept flag to 1
2. Wait for queue to become empty
3. Set shutdown flag to 1
4. Signal threads that wait on empty queue, so they can wake up, see shutdown flag
and exit.
5. Join all threads
6. Free whatever you have to free.
-----------------------------------------*/
void destroy_threadpool(threadpool* destroyme)
{
	if (destroyme == NULL)
	{
		perror("destroyme is null (cant free!)\n");
		return;
	}

	int i = 0;
	//1 By PDF

	//2 By PDF
	if (pthread_mutex_lock(&(destroyme->qlock)) != 0)
	{
		perror("Cant lock the mutex\n");
		return;
	}
	// flag_destroy = 1;
	destroyme->dont_accept = 1; // begun destroy
	if (destroyme->qsize != 0)
	{
   		if (pthread_cond_wait(&(destroyme->q_not_empty), &(destroyme->qlock)) != 0)
   		{
   			perror("error in destroy pthread_cond_wait");
   			return;
   		}

	}
   	//3 By PDF

   	destroyme->shutdown =1;
   	//4 By PDF
   	pthread_cond_broadcast(&destroyme->q_empty); // broadcast = all theards
 	if (pthread_mutex_unlock(&destroyme->qlock) != 0)
 	{
 		perror ("error in dispatch pthread_mutex_unlock");
 		return;
 	}
 	 // unlock
 	//5 By PDF
 	int rc = 0;

 	while(i < destroyme->num_threads)
 	{
 		rc = pthread_join(destroyme->threads[i], NULL);
 		if (rc!=0)
 		{
      		perror("prob to join threads in destroy\n");
      		return;
 		}
 		i++;
 	}
 	//6 By PDF
 	if (pthread_mutex_destroy(&destroyme->qlock) != 0)
 	{
 		perror ("error in dispatch pthread_mutex_destroy");
 		return;
 	}
 	if (pthread_cond_destroy(&destroyme->q_empty) != 0)
 	{
 		perror ("error in dispatch pthread_cond_destroy(q_empty)");
 		return;
 	}
 	if (pthread_cond_destroy(&destroyme->q_not_empty) != 0)
 	{
 		perror ("error in dispatch pthread_cond_destroy(q_not_empty)");
 		return;
 	}
  	free(destroyme->threads);
  	free(destroyme);
}
