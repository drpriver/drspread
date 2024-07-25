//
// Copyright © 2021-2024, David Priver <david@davidpriver.com>
//
#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H
#include <stdlib.h>
#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h> // sysconf
#include <pthread.h>
#if defined(__linux__)
#include <semaphore.h>
#elif defined(__APPLE__)
// semaphore_wait, semaphore_signal
#include <mach/semaphore.h>
// semaphore_create, destroy
#include <mach/task.h>
// mach_task_self
#include <mach/mach_init.h>
#endif
#elif defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#ifdef _MSC_VER
#pragma warning( disable : 5105)
#endif
#include <Windows.h>
#endif
#endif

#ifndef unhandled_error_condition
#include <assert.h>
#define unhandled_error_condition(x) assert(!(x))
#endif

#ifndef warn_unused

#if defined(__GNUC__) || defined(__clang__)
#define warn_unused __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define warn_unused
#else
#define warn_unused
#endif

#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#endif


//
// Implements a very basic portability layer to spawn a worker thread with an
// argument and then to wait for that thread to return.  Works on windows and
// posix.
//
// Example Usage:
//
//      THREADFUNC(worker);
//
//      typedef struct JobData {
//          const char* msg;
//          int whatever;
//      } JobData;
//
//      int main(void){
//          JobData data = {.msg="some very import information"};
//          ThreadHandle handle;
//          create_thread(&handle, worker, &data);
//          // No touchy the data until we join, the other thread
//          // is reading and possibly mutating it and we didn't set up
//          // any synchronization mechanism.
//
//          // ... do some main thread work ...
//
//          join_thread(handle);
//          if(data.whatever){
//             //  ... do whatever based on results of the worker thread ...
//          }
//          return 0;
//      }
//
//      THREADFUNC(worker){
//          JobData* data = thread_arg;
//          data->whatever = puts(data->msg);
//          // always return 0.
//          return 0;
//      }
//

//
// #define THREADFUNC(name) ret_type_differs (name)(void*_Nullable thread_arg)

//
// This structure is the handle to the thread. You will pass an unitialized one
// into the create_thread and it will store whatever it needs to within.  You
// can then later give it to join_thread to wait for the thread to finish.


typedef struct ThreadHandle ThreadHandle;
#ifdef _WIN32
typedef unsigned long ThreadReturnValue;
#else
typedef void*_Nullable ThreadReturnValue;
#endif


// Use this macro to portably define the entry point function to the thread.
// Can be used both in declaration context and in definition.  The function
// will have a single argument - thread_arg - which is a pointer to void. You
// decide if it can be NULL or not. As the return type differs between
// platforms, you should not use it for anything and should always return 0.
// Use the thread_arg to communicate a result back to the caller or set up some
// other message passing scheme.
//
//  Example:
//      THREADFUNC(worker);
//
//      THREADFUNC(worker){
//          JobData* data = thread_arg;
//          ... do some work ...
//          return 0;
//      }
//
#define THREADFUNC(name) ThreadReturnValue (name)(void*_Nullable thread_arg)
typedef THREADFUNC(thread_func);

//
// Creates and launches that thread, with the given thread func and argument.
// Initializes the given handle with the info that identifies that thread.
// thread_arg should be what the thread_func expects.
static warn_unused int create_thread(ThreadHandle* handle, thread_func* func, void*_Nullable thread_arg);
//
// Waits for the corresponding thread to finish.
// This is a synchronization event between the joiner and the joinee.
// (I know that is true for pthreads and assume it is true for Win32 as well.)
//
// Commented as it takes the handle by value.
// static void join_thread(ThreadHandle);

static int num_cpus(void);

#if defined(__linux__) || defined(__APPLE__)

typedef struct ThreadHandle ThreadHandle;
struct ThreadHandle {
    pthread_t thread;
};

typedef pthread_mutex_t LOCK_T;
static inline
void
LOCK_T_init(LOCK_T* lock){
    pthread_mutex_init(lock, NULL);
}

static inline
void
LOCK_T_lock(LOCK_T* lock){
    pthread_mutex_lock(lock);
}

static inline
void
LOCK_T_unlock(LOCK_T* lock){
    pthread_mutex_unlock(lock);
}


static
int
num_cpus(void){
    int num = sysconf(_SC_NPROCESSORS_ONLN);
    return num;
}

static
warn_unused
int
create_thread(ThreadHandle* handle, thread_func* func, void*_Nullable thread_arg){
    int err = pthread_create(&handle->thread, NULL, func, thread_arg);
    return err;
}

static
void
join_thread(ThreadHandle handle){
    int err = pthread_join(handle.thread, NULL);
    unhandled_error_condition(err != 0);
}

#elif defined(_WIN32)

typedef CRITICAL_SECTION LOCK_T;
static inline
void
LOCK_T_init(LOCK_T* lock){
    InitializeCriticalSection(lock);
}

static inline
void
LOCK_T_lock(LOCK_T* lock){
    EnterCriticalSection(lock);
}

static inline
void
LOCK_T_unlock(LOCK_T* lock){
    LeaveCriticalSection(lock);
}

typedef struct ThreadHandle ThreadHandle;
struct ThreadHandle {
    HANDLE thread;
};

static
int
num_cpus(void){
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    int num = sysinfo.dwNumberOfProcessors;
    return num;
}

static
warn_unused
int
create_thread(ThreadHandle* handle, thread_func* func, void*_Nullable thread_arg){
    handle->thread = CreateThread(NULL, 0, func, thread_arg, 0, NULL);
    return handle->thread == NULL;
}

static
void
join_thread(ThreadHandle handle){
    DWORD result = WaitForSingleObject(handle.thread, INFINITE);
    unhandled_error_condition(result != WAIT_OBJECT_0);
}

#elif defined(__wasm__)

typedef struct ThreadHandle ThreadHandle;
struct ThreadHandle {
    int unused;
};

static
warn_unused
int
create_thread(ThreadHandle* handle, thread_func* func, void*_Nullable thread_arg){
    (void)handle;
    (void)func;
    (void)thread_arg;
    return 1;
}

static
void
join_thread(ThreadHandle handle){
    (void)handle;
}
static int num_cpus(void){
    return 1;
}
#else
#error "Unhandled threading platform."
#endif

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#ifdef __clang__
#pragma clang diagnostic pop

#elif defined(__GNUC__)
#pragma GCC diagnostic pop

#endif

#endif
