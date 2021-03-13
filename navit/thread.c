/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2019 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

/** @file
 *
 * @brief Abstraction layer for system-specific thread routines.
 */

#include "thread.h"
#include <glib.h>
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#else
#include <errno.h>
#include <time.h>
#define _GNU_SOURCE
#endif
#include "debug.h"

#if HAVE_POSIX_THREADS
/**
 * @brief Describes the main function for a thread.
 */
struct thread_main_data {
    int (*main)(void *);       /**< The thread’s main function. */
    void * data;               /**< The argument for the function. */
};

/**
 * @brief Encapsulates the data structures for a pthread condition.
 */
struct thread_event_pthread {
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
};

/**
 * @brief A wrapper around the main thread function.
 *
 * This wraps the implementation-neutral main function into a function with the signature expected by the POSIX thread
 * library.
 *
 * @param data Pointer to a `struct thread_main_data` encapsulating the main function and its argument.
 */
static void *thread_main_wrapper(void * data) {
    struct thread_main_data * main_data = (struct thread_main_data *) data;
    void * ret = (void *) (main_data->main(main_data->data));
    return ret;
}

char * thread_format_error(int error) {
    switch(error) {
    case EPERM:
        return "EPERM (Operation not permitted)";
    case ENOENT:
        return "ENOENT (No such file or directory)";
    case ESRCH:
        return "ESRCH (No such process)";
    case EINTR:
        return "EINTR (Interrupted system call)";
    case EIO:
        return "EIO (I/O error)";
    case ENXIO:
        return "ENXIO (No such device or address)";
    case E2BIG:
        return "E2BIG (Argument list too long)";
    case ENOEXEC:
        return "ENOEXEC (Exec format error)";
    case EBADF:
        return "EBADF (Bad file number)";
    case ECHILD:
        return "ECHILD (No child processes)";
    case EAGAIN:
        return "EAGAIN (Try again)";
    case ENOMEM:
        return "ENOMEM (Out of memory)";
    case EACCES:
        return "EACCES (Permission denied)";
    case EFAULT:
        return "EFAULT (Bad address)";
    case ENOTBLK:
        return "ENOTBLK (Block device required)";
    case EBUSY:
        return "EBUSY (Device or resource busy)";
    case EEXIST:
        return "EEXIST (File exists)";
    case EXDEV:
        return "EXDEV (Cross-device link)";
    case ENODEV:
        return "ENODEV (No such device)";
    case ENOTDIR:
        return "ENOTDIR (Not a directory)";
    case EISDIR:
        return "EISDIR (Is a directory)";
    case EINVAL:
        return "EINVAL (Invalid argument)";
    case ENFILE:
        return "ENFILE (File table overflow)";
    case EMFILE:
        return "EMFILE (Too many open files)";
    case ENOTTY:
        return "ENOTTY (Not a typewriter)";
    case ETXTBSY:
        return "ETXTBSY (Text file busy)";
    case EFBIG:
        return "EFBIG (File too large)";
    case ENOSPC:
        return "ENOSPC (No space left on device)";
    case ESPIPE:
        return "ESPIPE (Illegal seek)";
    case EROFS:
        return "EROFS (Read-only file system)";
    case EMLINK:
        return "EMLINK (Too many links)";
    case EPIPE:
        return "EPIPE (Broken pipe)";
    case EDOM:
        return "EDOM (Math argument out of domain of func)";
    case ERANGE:
        return "ERANGE (Math result not representable)";
    case EDEADLK:
        return "EDEADLK (Resource deadlock would occur)";
    case ENAMETOOLONG:
        return "ENAMETOOLONG (File name too long)";
    case ENOLCK:
        return "ENOLCK (No record locks available)";
    default:
        return "unknown";
    }
}
#endif

#if HAVE_NAVIT_THREADS
/*
 * Wrapping the function in a conditional causes the build to fail if thread_new() is called on a
 * platform without thread support.
 */
thread *thread_new(int (*main)(void *), void * data, char * name) {
#if HAVE_POSIX_THREADS
    int err;
    thread * ret = g_new0(thread, 1);
    struct thread_main_data * main_data = g_new0(struct thread_main_data, 1);
    main_data->main = main;
    main_data->data = data;
    err = pthread_create(ret, NULL, thread_main_wrapper, (void *) main_data);
    if (err) {
        dbg(lvl_error, "error %d %s, thread=%p", err, thread_format_error(err), ret);
        g_free(ret);
        return NULL;
    }
#ifdef HAVE_PTHREAD_SETNAME_NP
    if (name) {
        err = pthread_setname_np(*ret, name);
        if (err)
            dbg(lvl_warning, "error %d %s, thread=%p", err, thread_format_error(err), ret);
    }
#endif
    return ret;
#else
    dbg(lvl_error, "call to thread_new() on a platform without thread support");
    return NULL;
#endif
}
#endif

void thread_destroy(thread* this_) {
#if HAVE_POSIX_THREADS
    g_free(this_);
#endif
}

void thread_sleep(long msec) {
#ifdef HAVE_API_WIN32_BASE
    Sleep(msec);
#else
    struct timespec req, rem;
    req.tv_sec = (msec /1000);
    req.tv_nsec = (msec % 1000) * 1000000;
    while ((nanosleep(&req, &rem) == -1) && errno == EINTR)
        req = rem;
#endif
}

void thread_exit(int result) {
#if HAVE_POSIX_THREADS
    pthread_exit((void *) result);
#else
    return;
#endif
}

int thread_join(thread * this_) {
#if HAVE_POSIX_THREADS
    void * ret;
    int err = pthread_join(*this_, &ret);
    if (err) {
        dbg(lvl_error, "error %d %s, thread=%p", err, thread_format_error(err), this_);
        return -1;
    }
    return (int) ret;
#else
    // TODO Win32 WaitForSingleObject(thread)
    return -1;
#endif
}

thread_id thread_get_id(void) {
#if HAVE_POSIX_THREADS
    return pthread_self();
#else
    return 0;
#endif
}

thread_event *thread_event_new(void) {
#if HAVE_POSIX_THREADS
    thread_event *ret = g_new0(thread_event, 1);
    ret->cond = g_new0(pthread_cond_t, 1);
    pthread_cond_init(ret->cond, NULL);
    ret->mutex = g_new0(pthread_mutex_t, 1);
    pthread_mutex_init(ret->mutex, NULL);
    return ret;
#else
    // TODO Win32 CreateEvent (auto-reset)
    return 0;
#endif
}

void thread_event_destroy(thread_event *this_) {
#if HAVE_POSIX_THREADS
    pthread_cond_destroy(this_->cond);
    g_free(this_->cond);
    pthread_mutex_destroy(this_->mutex);
    g_free(this_->mutex);
    g_free(this_);
#else
    // TODO Win32?
    return;
#endif
}

void thread_event_signal(thread_event *this_) {
#if HAVE_POSIX_THREADS
    pthread_mutex_lock(this_->mutex);
    /*
     * Using pthread_cond_signal as it wakes just one thread, similar to an auto-reset event on WinAPI
     * (pthread_cond_broadcast would wake all waiting threads).
     */
    pthread_cond_signal(this_->cond);
    pthread_mutex_unlock(this_->mutex);
#else
    // TODO Win32: SetEvent
    return;
#endif
}

void thread_event_reset(thread_event *this_) {
#if HAVE_POSIX_THREADS
    return;
#else
    // TODO Win32: ResetEvent
    return;
#endif
}

void thread_event_wait(thread_event *this_, long msec) {
#if HAVE_POSIX_THREADS
    struct timeval tp;
    struct timespec ts;
    pthread_mutex_lock(this_->mutex);
    if (msec < 0)
        pthread_cond_wait(this_->cond, this_->mutex);
    else {
        gettimeofday(&tp, NULL);
        /* if msec is 1 s or more, add integer part to tv_sec */
        ts.tv_sec = tp.tv_sec + floor(msec / 1000);
        /* for now, these are really µsec, not nsec, to prevent overflow */
        ts.tv_nsec = tp.tv_usec + (msec % 1000) * 1000000;
        /* if tv_nsec is 1s or more, move integer second part to tv_sec */
        ts.tv_sec += floor(ts.tv_nsec / 1000000);
        ts.tv_nsec %= 1000000;
        /* and finally, convert µsec to nsec */
        ts.tv_nsec *= 1000;

        pthread_cond_timedwait(this_->cond, this_->mutex, &ts);
    }
    pthread_mutex_unlock(this_->mutex);
#else
    // TODO Win32 WaitForSingleObject(event, msec)
    return;
#endif
}

thread_lock *thread_lock_new(void) {
#if HAVE_POSIX_THREADS
    thread_lock *ret = g_new0(thread_lock, 1);
    int err = pthread_rwlock_init(ret, NULL);
    if (err) {
        dbg(lvl_error, "error %d %s, lock=%p", err, thread_format_error(err), ret);
        g_free(ret);
        return NULL;
    }
    return ret;
#else
    // TODO Win32 CreateMutex (SRWLock requires 6.0+)
    return NULL;
#endif
}

void thread_lock_destroy(thread_lock *this_) {
#if HAVE_POSIX_THREADS
    int err = pthread_rwlock_destroy(this_);
    if (err)
        dbg(lvl_error, "error %d %s, lock=%p", err, thread_format_error(err), this_);
    g_free(this_);
#endif
}

void thread_lock_acquire_read(thread_lock *this_) {
#if HAVE_POSIX_THREADS
    int err = pthread_rwlock_rdlock(this_);
    if (err) {
        dbg(lvl_error, "error %d %s, lock=%p", err, thread_format_error(err), this_);
        abort();
    }
#endif
}

int thread_lock_try_read(thread_lock *this_) {
#if HAVE_POSIX_THREADS
    int err = pthread_rwlock_tryrdlock(this_);
    if (err) {
        dbg(lvl_debug, "error %d %s, lock=%p", err, thread_format_error(err), this_);
        return 0;
    }
    return 1;
#else
    return 1;
#endif
}

void thread_lock_release_read(thread_lock *this_) {
#if HAVE_POSIX_THREADS
    int err = pthread_rwlock_unlock(this_);
    if (err) {
        dbg(lvl_error, "error %d %s, lock=%p", err, thread_format_error(err), this_);
        abort();
    }
#endif
}

void thread_lock_acquire_write(thread_lock *this_) {
#if HAVE_POSIX_THREADS
    int err = pthread_rwlock_wrlock(this_);
    if (err) {
        dbg(lvl_error, "error %d %s, lock=%p", err, thread_format_error(err), this_);
        abort();
    }
#endif
}

int thread_lock_try_write(thread_lock *this_) {
#if HAVE_POSIX_THREADS
    int err = pthread_rwlock_trywrlock(this_);
    if (err) {
        dbg(lvl_debug, "error %d %s, lock=%p", err, thread_format_error(err), this_);
        return 0;
    }
    return 1;
#else
    return 1;
#endif
}

void thread_lock_release_write(thread_lock *this_) {
#if HAVE_POSIX_THREADS
    int err = pthread_rwlock_unlock(this_);
    if (err) {
        dbg(lvl_error, "error %d %s, lock=%p", err, thread_format_error(err), this_);
        abort();
    }
#endif
}
