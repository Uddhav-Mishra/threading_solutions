
/*

Given a string S, N threads will be printing K chars in an ordered manner.
Each thread would print X no. of times and would print K chars starting from where the previous thread left.
Thread X should print only after X-1 has finished
its current iteration of printing. When N'th thread has finished its current iteration
thread 1 should start if more iterations are left.

Note :
No thread should starve, every thread should get its turn to print for the current iteration.
Each thread should print in the sequential manner and next to the characters printed by the
previous threads as expected. Refer to below examples for
details.

Ex.

Input 1:
S = "ABCDEF"
N = 2
K = 2
T = 3

Output 1:
Thread1: AB
Thread2: CD
Thread1: EF
Thread2: AB
Thread1: CD
Thread2: EF

Input 2:
S = "ABCDEF"
N = 4
K = 5
T = 2

Output 2:
Thread1: ABCDE
Thread2: FABCD
Thread3: EFABC
Thread1: DEFAB
Thread2: CDEFA
Thread3: BCDEF



Compilation and Execution: 
g++ -std=c++11 -pthread string_print_by_threads.cpp
./a.out

Some pthread construct descriptions:

pthread_cond_t : 
a synchronization device that allows threads to suspend execution and relinquish the processors until
some predicate on shared data is satisfied. The basic operations on conditions are: signal the condition
(when the predicate becomes true), and wait for the condition, suspending the thread execution until another thread signals the condition.

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex):
atomically unlocks the mutex (as per pthread_unlock_mutex) and waits for the condition variable cond to be signaled. 
The thread execution is suspended and does not consume any CPU time until the condition variable is signaled.
The mutex must be locked by the calling thread on entrance to pthread_cond_wait. Before returning to the calling thread,
pthread_cond_wait re-acquires mutex (as per pthread_lock_mutex).

int pthread_cond_signal(pthread_cond_t *cond);:
pthread_cond_signal restarts one of the threads that are waiting on the condition variable cond.
If no threads are waiting on cond, nothing happens.
If several threads are waiting on cond, exactly one is restarted, but it is not specified which.

*/


#include <unistd.h>
#include <iostream>

using namespace std;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int N;
string s ;
int K;
int T;

struct thread_arg
{
    int index;
    pthread_cond_t *waitCond;
    pthread_cond_t *signalCond;
};

void* threadManager(void* pArg)
{
    struct thread_arg *arg = (struct thread_arg *)pArg;
    int i=K*arg->index, sig_ret,tu = 0;
    while(tu<T)
    {
        pthread_mutex_lock(&mutex1);
        printf("Thread%d: ",(arg->index+1));
        for(int j = i ; j< i+K ; j++)cout << s[j%s.size()];  
        cout << endl;
		if (N > 1){
          pthread_cond_signal(arg->signalCond);
          while( pthread_cond_wait(arg->waitCond, &mutex1) != 0 ){}
        }
        pthread_mutex_unlock(&mutex1);
	    i += K*N;
        tu++;
    }

    pthread_cond_signal(arg->signalCond);
    return NULL;    
}


int main(int argc,char *argv[])
{   
    s = "ABCDEF";
    N = 2;
    K = 2;
    T = 3;		

    pthread_cond_t  cond[N];
    pthread_t ThreadId[N];
    struct thread_arg ThreadArg[N];

    for (int i =0; i < N ; ++i) {
        cond[i] = PTHREAD_COND_INITIALIZER;
    }

    for (int i =0; i< N ; i++)
    {
        ThreadArg[i].index = i;
        ThreadArg[i].waitCond = &cond[i];
        if (i == N-1)
            ThreadArg[i].signalCond = &cond[0];
        else
            ThreadArg[i].signalCond = &cond[i+1];

        pthread_create(&ThreadId[i], NULL, &threadManager,(void*)&ThreadArg[i]);
        usleep(500);
    }
    for (int i =0; i< N ; i++)
        pthread_join(ThreadId[i],NULL);

    return 0;
}
