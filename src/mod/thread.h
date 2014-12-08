/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2014 by Thomas Poechtrager                           *
 *  t.poechtrager@gmail.com                                            *
 *                                                                     *
 *  This program is free software; you can redistribute it and/or      *
 *  modify it under the terms of the GNU General Public License        *
 *  as published by the Free Software Foundation; either version 2     *
 *  of the License, or (at your option) any later version.             *
 *                                                                     *
 *  This program is distributed in the hope that it will be useful,    *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *  GNU General Public License for more details.                       *
 *                                                                     *
 *  You should have received a copy of the GNU General Public License  *
 *  along with this program; if not, write to the Free Software        *
 *  Foundation, Inc.,                                                  *
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.      *
 ***********************************************************************/

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#include <signal.h>
#endif //_WIN32

//#define CHECKMUTEX

#if defined(_MSC_VER) || \
    ( defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && \
     !defined(__INTEL_COMPILER) && !defined(__clang__) )
#include <atomic> // TODO: replace this
using std::atomic;
#else
#if defined(__clang__) && !CLANG_VERSION_AT_LEAST(3, 4, 0)
#define ATOMIC_NO_INLINE __attribute__((noinline))
#else
#define ATOMIC_NO_INLINE
#endif

template<class T> class atomic : noncopyable
{
public:
    void store(T val) ATOMIC_NO_INLINE
    {
        __atomic_store(&var, &val, __ATOMIC_SEQ_CST);
    }
    T load()
    {
        T val;
        __atomic_load(&var, &val, __ATOMIC_SEQ_CST);
        return val;
    }
    operator T() { return load(); }
    T operator++(int) { return __atomic_fetch_add(&var, 1, __ATOMIC_SEQ_CST); }
    T operator--(int) { return __atomic_fetch_sub(&var, 1, __ATOMIC_SEQ_CST); }
    T operator+=(T val) { return __atomic_fetch_add(&var, val, __ATOMIC_SEQ_CST); }
    T operator-=(T val) { return __atomic_fetch_sub(&var, val, __ATOMIC_SEQ_CST); }
    void operator=(T val) { store(val); }
    atomic(T val = T(0)) { store(val); }
private:
    T var;
};
#endif

class sdlmutex : noncopyable
{
public:
    sdlmutex() : m(SDL_CreateMutex()) { if (!m.load()) abort(); }
    ~sdlmutex()
    {
#ifdef CHECKMUTEX
        assert(!islocked && "attempt to destroy locked mutex");
#endif //CHECKMUTEX
        SDL_DestroyMutex(m);
    }
    void lock()
    {
        SDL_mutexP(m);
#ifdef CHECKMUTEX
        islocked = true;
#endif //CHECKMUTEX
    }
    void unlock()
    {
#ifdef CHECKMUTEX
        assert(islocked && "mutex already unlocked");
#endif //CHECKMUTEX
        SDL_mutexV(m);
#ifdef CHECKMUTEX
        islocked = false;
#endif //CHECKMUTEX
    }
    SDL_mutex *getmutex() { return m; }
private:
    atomic<SDL_mutex*> m;
#ifdef CHECKMUTEX
    atomic<bool> islocked{false};
#endif //CHECKMUTEX
};

class sdlcondition : noncopyable
{
public:
    sdlcondition() : c(SDL_CreateCond()) { if (!c.load()) abort(); }
    ~sdlcondition() { SDL_DestroyCond(c); }
    void signalone() { SDL_CondSignal(c); }
    void signalall() { SDL_CondBroadcast(c); }
    void wait(sdlmutex &m) { SDL_CondWait(c, m.getmutex()); }
    void wait(sdlmutex &m, uint timeout) { SDL_CondWaitTimeout(c, m.getmutex(), timeout); }
    SDL_cond *getcond() { return c; }
private:
    atomic<SDL_cond*> c;
};

class sdlthread : noncopyable
{
public:
    typedef int (*threadfn)(void *data);

    sdlthread(threadfn fn, void *data = NULL) : joined(false)
    {
#ifdef _WIN32
        // SDL_CreateThread deadlocks somewhere in the kernel on windows, don't know why ...
        t = (void*)_beginthreadex(NULL, 0, (uint (__stdcall *)(void *))fn, data, 0, NULL);
#else
        t = SDL_CreateThread(fn, "sdlthread", data);
#endif //_WIN32

#ifdef PLUGIN
        if (!t) abort();
#else
        if (!t) fatal("couldn't create thread");
#endif //PLUGIN
    }
    ~sdlthread() { join(); }

    void join()
    {
        m.lock();

        if (joined)
        {
            m.unlock();
            return;
        }

#ifdef WIN32
        WaitForSingleObject(t, INFINITE);
        CloseHandle(t);
#else
        SDL_WaitThread((SDL_Thread*)t, NULL);
#endif //_WIN32

        t = NULL;
        joined = true;

        m.unlock();
    }
    bool isjoined()
    {
        m.lock();
        bool j = joined;
        m.unlock();
        return j;
    }

private:
    sdlthread() {}
    bool joined;
    sdlmutex m;
    void *t;
};

#define Destroy_SDL_Mutex(_m) \
    do { if (_m != NULL) { SDL_DestroyMutex(_m); _m = NULL; } } while (0)

#define Destroy_SDL_Cond(_c) \
    do { if (_c != NULL) { SDL_DestroyCond(_c); _c = NULL; } } while (0)

struct SDL_Mutex_Locker
{
    SDL_Mutex_Locker(SDL_mutex *m) : mutex(m) { SDL_LockMutex(mutex); }
    SDL_Mutex_Locker(sdlmutex &sm) : mutex(sm.getmutex()) { SDL_LockMutex(mutex); }
    ~SDL_Mutex_Locker() { SDL_UnlockMutex(mutex); }
    SDL_mutex *mutex;
};

#ifdef _WIN32
typedef HANDLE SYS_ThreadHandle;
#else
typedef pthread_t SYS_ThreadHandle;
#endif // _WIN32

static const int SDL_THREAD_KILLED = 0xFEFEFEFE;

struct SDL_Thread
{
    SDL_threadID threadid;
    SYS_ThreadHandle handle;
    int status;
};

static inline void SDL_KillThread(SDL_Thread *thread)
{
#ifdef _WIN32
    TerminateThread(thread->handle, 0);
#else
#ifdef PTHREAD_CANCEL_ASYNCHRONOUS
    pthread_cancel(thread->handle);
#else
#error PTHREAD_CANCEL_ASYNCHRONOUS not defined
#endif
#endif // _WIN32
    memset(&thread->handle, 0, sizeof(thread->handle));
    thread->status = SDL_THREAD_KILLED;
}
