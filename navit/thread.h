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
 * @brief Thread abstraction layer for Navit.
 *
 * This file provides a cross-platform thread API for Navit. It is not necessarily feature-complete—if Navit doesn’t
 * need a certain feature, it will not be included.
 *
 * It is permitted (and even desirable) to include this header file on platforms which have no support for threads:
 * this will set the `HAVE_NAVIT_THREADS` constant to 0, allowing for the use of preprocessor conditionals to write
 * code which will run in both single-threaded and multi-threaded environments.
 *
 * On platforms for which Navit does not have thread support, functions defined in this header file are no-ops if they
 * are not meaningful in a single-threaded environment. For example, attempting to acquire a lock is not necessary if
 * there are no other threads which might already be holding the lock. Mapping these functions to no-ops allows for
 * better code legibility, as it eliminates the need to wrap these calls into conditionals. Where they occur next to
 * sections wrapped in conditionals, they may be placed either inside or outside the conditional section. Consistency
 * is advised, though.
 *
 * On the other hand, operations such as attempting to spawn a new thread will cause a compiler error in a
 * single-threaded environment, as there is no appropriate “translation” for this operation (the developer would have
 * to choose an appropriate way of running the code on the main thread, possibly with further refactoring to avoid
 * locking up the program, thus attempting to spawn a thread is an error).
 *
 * Types defined in this header are intended to be used as pointers. Constructor functions allocate memory and return
 * pointers, and other functions expect pointers as well. On platforms without thread support, types defined here map
 * to `void` and cannot be instantiated directly.
 *
 * Deadlocks are currently not taken into account. Depending on the lower layer, threads involved in a deadlock may
 * either lock up forever, or log an error and use the locked resource anyway (resulting in inconsistencies later on).
 * This needs to be fixed in a later release and may involve API changes.
 *
 * Use of other thread APIs in Navit is generally discouraged. An exception to this are platform-specific modules:
 * these can use platform-specific APIs to spawn extra threads and synchronize with them, as long as they are entirely
 * contained within the module and no code outside the module ever needs to synchronize with them.
 */

#ifndef NAVIT_THREAD_H
#define NAVIT_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#ifdef HAVE_API_WIN32_BASE
#undef HAVE_NAVIT_THREADS
#warning "threads are not supported on this platform, building a single-threaded version"
#include <windows.h>
#else
#define HAVE_POSIX_THREADS 1
#define HAVE_NAVIT_THREADS 1
#include <pthread.h>
#endif


#if HAVE_POSIX_THREADS
#define thread pthread_t
#define thread_lock pthread_rwlock_t
#define thread_id pthread_t
#else
#define thread int
#define thread_lock int
#define thread_id int
#endif

/**
 * @brief Creates and spawns a new thread.
 *
 * The new thread will terminate if its `main` function returns or the process exits.
 *
 * This function must be surrounded with an `#if HAVE_NAVIT_THREADS` conditional, else use of this function will result
 * in a compiler error when building for a platform without thread support. An alternative way of running the thread
 * code is usually necessary for these platforms.
 *
 * @param main The main function for the new thread, see description
 * @param data Data passed to the main function
 * @param name A display name for the thread (may not be supported on all platforms)
 *
 * @return The new thread, or NULL if an error occurred.
 */
thread *thread_new(int (*main)(void *), void * data, char * name);

/**
 * @brief Frees all resources associated with the thread.
 *
 * If Navit was built without thread support, this is a no-op.
 */
void thread_destroy(thread* this_);

/**
 * @brief Pauses the current thread for a set amount of time.
 *
 * If Navit was built without thread support, this will make Navit sleep for the amount of time requested.
 *
 * @param msec The number of milliseconds to sleep for
 */
void thread_sleep(long msec);

/**
 * @brief Exits the current thread.
 *
 * The exit code can be obtained by calling `thread_join()` on the thread.
 *
 * If Navit was built without thread support, this is a no-op.
 *
 * @param result The exit code
 */
void thread_exit(int result);

/**
 * @brief Joins a thread, i.e. blocks until the thread has finished.
 *
 * If Navit was built without thread support, this function will return -1 immediately.
 *
 * @return The thread’s exit value, -1 if an error was encountered.
 */
int thread_join(thread * this_);

/**
 * @brief Returns the ID of the calling thread.
 *
 * If Navit was built without thread support, this function will return 0.
 */
thread_id thread_get_id(void);

/**
 * @brief Creates a new lock.
 *
 * The caller is responsible for freeing up the lock with `thread_lock_destroy()` when it is no longer needed.
 *
 * If Navit was built without thread support, this is a no-op and NULL will be returned.
 */
thread_lock *thread_lock_new(void);

/**
 * @brief Frees all resources associated with the lock.
 *
 * Prior to calling this function, the caller must first ensure the lock is no longer reachable by any other thread,
 * and then release it. A lock can be made unreachable by creating a local reference to it (or the structure containing
 * it), and setting all other references to NULL.
 *
 * Unless pointers to the structure are initialized before they can be dereferenced by threads other than the main (UI)
 * thread, and never changed afterwards, each pointer needs to be protected with a lock of its own. The lock must be
 * acquired for reading in order to dereference the pointer, and acquired for writing in order to change the pointer.
 * If multiple pointers to the same struct are allowed, each pointer must be protected in this manner. The struct must
 * then keep a thread-safe reference count and free its memory when the last reference is released.
 *
 * The following would be incorrect, because another thread could still attempt to acquire the lock while it is being
 * destroyed:
 * {@code
 * thread_lock_release_write(foo->lock);    // The lock is now released and any other thread can take it
 * thread_lock_destroy(foo->lock);          // By the time we get here, another thread might have acquired the lock
 * foo->lock = NULL;
 * }
 *
 * The correct approach would be:
 * {@code
 * thread_lock * lock = foo->lock;
 * thread_lock_acquire_write(lock_for_foo); // Acquire lock for the foo pointer
 * foo->lock = NULL;
 * thread_lock_release_write(lock_for_foo);
 * thread_lock_release_write(lock);         // Safe if no other references to the lock exist
 * thread_lock_destroy(lock);
 * }
 *
 * If Navit was built without thread support, this is a no-op. If `lock` is NULL on a platform with thread support,
 * the behavior is undefined.
 */
void thread_lock_destroy(thread_lock *this_);

/**
 * @brief Acquires a read lock for the current thread.
 *
 * Read locks are recursive, i.e. one thread can acquire the same read lock multiple times. Each release will undo
 * exactly one lock operation, i.e. if a read lock was acquired n times, it must be released n times before another
 * thread can acquire a write lock.
 *
 * If another thread is currently holding the same lock for writing, the calling thread will block until the lock can
 * be acquired. If lock acquisition fails for any reason (including a deadlock), the process will abort.
 *
 * If Navit was built without thread support, this is a no-op.
 */
void thread_lock_acquire_read(thread_lock *this_);

/**
 * @brief Tries to acquire a read lock for the current thread.
 *
 * If the lock can be successfully acquired, this function returns TRUE and otherwise behaves identically to
 * `thread_lock_acquire_read()`. If acquiring the lock fails for whatever reason, it never blocks but returns FALSE
 * immediately. The caller must evaluate the return value. If the result was FALSE, indicating the lock was not
 * obtained, it needs to recover from the situation and refrain from operations which the lock protects.
 *
 * If Navit was built without thread support, this is a no-op and the result will always be TRUE.
 */
int thread_lock_try_read(thread_lock *this_);

/**
 * @brief Releases a read lock for the current thread.
 *
 * If the lock cannot be released for any reason, the process will abort.
 *
 * If Navit was built without thread support, this is a no-op.
 */
void thread_lock_release_read(thread_lock *this_);

/**
 * @brief Acquires a write lock for the current thread.
 *
 * Write locks, unlike read locks, are not recursive, i.e. even the same thread cannot acquire the same write lock more
 * than once.
 *
 * If another thread is currently holding the same lock, the calling thread will block until the lock can be acquired.
 * If lock acquisition fails for any reason (including a deadlock), the process will abort.
 *
 * If Navit was built without thread support, this is a no-op.
 */
void thread_lock_acquire_write(thread_lock *this_);

/**
 * @brief Tries to acquire a write lock for the current thread.
 *
 * If the lock can be successfully acquired, this function returns TRUE and otherwise behaves identically to
 * `thread_lock_acquire_write()`. If acquiring the lock fails for whatever reason, it never blocks but returns FALSE
 * immediately. The caller must evaluate the return value. If the result was FALSE, indicating the lock was not
 * obtained, it needs to recover from the situation and refrain from operations which the lock protects.
 *
 * If Navit was built without thread support, this is a no-op and the result will always be TRUE.
 */
int thread_lock_try_write(thread_lock *this_);

/**
 * @brief Releases a write lock for the current thread.
 *
 * If the lock cannot be released for any reason, the process will abort.
 *
 * If Navit was built without thread support, this is a no-op.
 */
void thread_lock_release_write(thread_lock *this_);


#ifdef __cplusplus
}
#endif

#endif /* NAVIT_THREAD_H */
