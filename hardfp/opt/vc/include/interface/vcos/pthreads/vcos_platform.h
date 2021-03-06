/*
 * Copyright (c) 2010-2011 Broadcom. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*=============================================================================
VideoCore OS Abstraction Layer - pthreads types
=============================================================================*/

/* Do not include this file directly - instead include it via vcos.h */

/** @file
  *
  * Pthreads implementation of VCOS.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>


#define VCOS_HAVE_RTOS         1
#define VCOS_HAVE_SEMAPHORE    1
#define VCOS_HAVE_EVENT        1
#define VCOS_HAVE_QUEUE        0
#define VCOS_HAVE_LEGACY_ISR   0
#define VCOS_HAVE_TIMER        1
#define VCOS_HAVE_CANCELLATION_SAFE_TIMER 1
#define VCOS_HAVE_MEMPOOL      0
#define VCOS_HAVE_ISR          0
#define VCOS_HAVE_ATOMIC_FLAGS 1
#define VCOS_HAVE_THREAD_AT_EXIT        1
#define VCOS_HAVE_ONCE         1
#define VCOS_HAVE_BLOCK_POOL   1
#define VCOS_HAVE_FILE         0
#define VCOS_HAVE_PROC         0
#define VCOS_HAVE_CFG          0
#define VCOS_HAVE_ALIEN_THREADS  1
#define VCOS_HAVE_CMD          1
#define VCOS_HAVE_EVENT_FLAGS  1
#define VCOS_WANT_LOG_CMD      0    /* User apps should do their own thing */

#define VCOS_ALWAYS_WANT_LOGGING

#ifdef __linux__
#define VCOS_HAVE_BACKTRACE    1
#endif

#define VCOS_SO_EXT  ".so"

/* Linux/pthreads seems to have different timer characteristics */
#define VCOS_TIMER_MARGIN_EARLY 0
#define VCOS_TIMER_MARGIN_LATE 15

typedef sem_t                 VCOS_SEMAPHORE_T;
typedef sem_t                 VCOS_EVENT_T;
typedef uint32_t              VCOS_UNSIGNED;
typedef uint32_t              VCOS_OPTION;
typedef pthread_key_t         VCOS_TLS_KEY_T;
typedef pthread_once_t        VCOS_ONCE_T;

typedef struct VCOS_LLTHREAD_T
{
   pthread_t thread; // Must be first field.
} VCOS_LLTHREAD_T;

/* VCOS_CASSERT(offsetof(VCOS_LLTHREAD_T, thread) == 0); */

#ifndef VCOS_USE_VCOS_FUTEX
typedef pthread_mutex_t       VCOS_MUTEX_T;
#else
#include "vcos_futex_mutex.h"
#endif /* VCOS_USE_VCOS_FUTEX */

#define VCOS_ONCE_INIT        PTHREAD_ONCE_INIT

#if defined(__arm__) && !defined(_HAVE_TIMER_T) && !defined(ANDROID)
typedef __timer_t timer_t;
#endif
typedef struct VCOS_TIMER_T
{
   pthread_t thread;                      /**< id of the timer thread */

   pthread_mutex_t lock;                  /**< lock protecting all other members of the struct */
   pthread_cond_t settings_changed;       /**< cond. var. for informing the timer thread about changes*/
   int quit;                              /**< non-zero if the timer thread is requested to quit*/

   struct timespec expires;               /**< absolute time of next expiration, or 0 if disarmed*/

   void (*orig_expiration_routine)(void*);/**< the expiration routine provided by the user of the timer*/
   void *orig_context;                    /**< the context for exp. routine provided by the user*/

} VCOS_TIMER_T;

/** Thread attribute structure. Don't use pthread_attr directly, as
  * the calls can fail, and inits must match deletes.
  */
typedef struct VCOS_THREAD_ATTR_T
{
   void *ta_stackaddr;
   VCOS_UNSIGNED ta_stacksz;
   VCOS_UNSIGNED ta_priority;
   VCOS_UNSIGNED ta_affinity;
   VCOS_UNSIGNED ta_timeslice;
   VCOS_UNSIGNED legacy;
} VCOS_THREAD_ATTR_T;

/** Called at thread exit.
  */
typedef struct VCOS_THREAD_EXIT_T
{
   void (*pfn)(void *);
   void *cxt;
} VCOS_THREAD_EXIT_T;
#define VCOS_MAX_EXIT_HANDLERS  4

typedef struct VCOS_THREAD_T
{
   pthread_t thread;             /**< The thread itself */
   VCOS_THREAD_ENTRY_FN_T entry; /**< The thread entry point */
   void *arg;                    /**< The argument to be passed to entry */
   VCOS_SEMAPHORE_T suspend;     /**< For support event groups and similar - a per thread semaphore */

   VCOS_TIMER_T task_timer;
   int task_timer_created;       /**< non-zero if the task timer has already been created*/
   void (*orig_task_timer_expiration_routine)(void*);
   void *orig_task_timer_context;

   VCOS_UNSIGNED legacy;
   char name[16];                /**< Record the name of this thread, for diagnostics */
   VCOS_UNSIGNED dummy;          /**< Dummy thread created for non-vcos created threads */

   /** Callback invoked at thread exit time */
   VCOS_THREAD_EXIT_T at_exit[VCOS_MAX_EXIT_HANDLERS];
} VCOS_THREAD_T;

#ifdef VCOS_PTHREADS_WANT_HISR_EMULATION

typedef struct
{
   VCOS_THREAD_T thread;
   char stack[1024];
   VCOS_SEMAPHORE_T waitsem;
} VCOS_HISR_T;

#endif

#define VCOS_SUSPEND          -1
#define VCOS_NO_SUSPEND       0

#define VCOS_START 1
#define VCOS_NO_START 0

#define VCOS_THREAD_PRI_MIN   (sched_get_priority_min(SCHED_OTHER))
#define VCOS_THREAD_PRI_MAX   (sched_get_priority_max(SCHED_OTHER))

#define VCOS_THREAD_PRI_INCREASE (1)
#define VCOS_THREAD_PRI_HIGHEST  VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_LOWEST   VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_NORMAL ((VCOS_THREAD_PRI_MAX+VCOS_THREAD_PRI_MIN)/2)
#define VCOS_THREAD_PRI_BELOW_NORMAL (VCOS_THREAD_PRI_NORMAL-VCOS_THREAD_PRI_INCREASE)
#define VCOS_THREAD_PRI_ABOVE_NORMAL (VCOS_THREAD_PRI_NORMAL+VCOS_THREAD_PRI_INCREASE)
#define VCOS_THREAD_PRI_REALTIME VCOS_THREAD_PRI_MAX

#define _VCOS_AFFINITY_DEFAULT 0
#define _VCOS_AFFINITY_CPU0    0x100
#define _VCOS_AFFINITY_CPU1    0x200
#define _VCOS_AFFINITY_MASK    0x300
#define VCOS_CAN_SET_STACK_ADDR  0

#define VCOS_TICKS_PER_SECOND _vcos_get_ticks_per_second()

#include "interface/vcos/generic/vcos_generic_event_flags.h"
#include "interface/vcos/generic/vcos_generic_blockpool.h"
#include "interface/vcos/generic/vcos_mem_from_malloc.h"

/** Convert errno values into the values recognized by vcos */
VCOSPRE_ VCOS_STATUS_T vcos_pthreads_map_error(int error);
VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_pthreads_map_errno(void);

/** Register a function to be called when the current thread exits.
  */
extern VCOS_STATUS_T vcos_thread_at_exit(void (*pfn)(void*), void *cxt);

extern uint32_t _vcos_get_ticks_per_second(void);

/**
 * Set to 1 by default when ANDROID is defined. Allows runtime
 * switching for console apps.
 */
extern int vcos_use_android_log;

typedef struct {
   VCOS_MUTEX_T mutex;
   uint32_t flags;
} VCOS_ATOMIC_FLAGS_T;

#if defined(VCOS_INLINE_BODIES)

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 1


/*
 * Counted Semaphores
 */
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   int ret;
   /* gdb causes sem_wait() to EINTR when a breakpoint is hit, retry here */
   while ((ret = sem_wait(sem)) == -1 && errno == EINTR)
      continue;
   vcos_assert(ret==0);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   int ret;
   while ((ret = sem_trywait(sem)) == -1 && errno == EINTR)
      continue;
   if (ret == 0)
      return VCOS_SUCCESS;
   else if (errno == EAGAIN)
      return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

/**
  * \brief Wait on a semaphore with a timeout.
  *
  * Note that this function may not be implemented on all
  * platforms, and may not be efficient on all platforms
  * (see comment in vcos_semaphore_wait)
  *
  * Try to obtain the semaphore. If it is already taken, return
  * VCOS_EAGAIN.
  * @param sem Semaphore to wait on
  * @param timeout Number of milliseconds to wait before
  *                returning if the semaphore can't be acquired.
  * @return VCOS_SUCCESS - semaphore was taken.
  *         VCOS_EAGAIN - could not take semaphore (i.e. timeout
  *         expired)
  *         VCOS_EINVAL - Some other error (most likely bad
  *         parameters).
  */
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait_timeout(VCOS_SEMAPHORE_T *sem, VCOS_UNSIGNED timeout) {
   struct timespec ts;
   int ret;
   ts.tv_sec  = timeout/1000;
   ts.tv_nsec = (timeout%1000)*1000*1000;
   ret = sem_timedwait( sem, &ts );
   if (ret == 0)
      return VCOS_SUCCESS;
   else if (ret == ETIMEDOUT)
      return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(VCOS_SEMAPHORE_T *sem,
                                    const char *name,
                                    VCOS_UNSIGNED initial_count) {
   int rc = sem_init(sem, 0, initial_count);
   (void)name;
   if (rc != -1) return VCOS_SUCCESS;
   else return vcos_pthreads_map_errno();
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
   int rc = sem_destroy(sem);
   vcos_assert(rc != -1);
   (void)rc;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   int rc = sem_post(sem);
   vcos_assert(rc == 0);
   (void)rc;
   return VCOS_SUCCESS;
}

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/


extern VCOS_THREAD_T *vcos_dummy_thread_create(void);
extern pthread_key_t _vcos_thread_current_key;
extern uint32_t vcos_getmicrosecs_internal(void);

VCOS_INLINE_IMPL
uint32_t vcos_getmicrosecs(void) { return vcos_getmicrosecs_internal(); }

VCOS_INLINE_IMPL
VCOS_THREAD_T *vcos_thread_current(void) {
   void *ret = pthread_getspecific(_vcos_thread_current_key);
   if (ret == NULL)
   {
      ret = vcos_dummy_thread_create();
   }

#ifdef __cplusplus
   return static_cast<VCOS_THREAD_T*>(ret);
#else
   return (VCOS_THREAD_T *)ret;
#endif
}

VCOS_INLINE_IMPL
void vcos_sleep(uint32_t ms) {
   struct timespec ts;
   ts.tv_sec = ms/1000;
   ts.tv_nsec = ms % 1000 * (1000000);
   nanosleep(&ts, NULL);
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setstack(VCOS_THREAD_ATTR_T *attr, void *addr, VCOS_UNSIGNED sz) {
   attr->ta_stackaddr = addr;
   attr->ta_stacksz = sz;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setstacksize(VCOS_THREAD_ATTR_T *attr, VCOS_UNSIGNED sz) {
   attr->ta_stacksz = sz;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setpriority(VCOS_THREAD_ATTR_T *attr, VCOS_UNSIGNED pri) {
   (void)attr;
   (void)pri;
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED p) {
   /* not implemented */
   (void)thread;
   (void)p;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(VCOS_THREAD_T *thread) {
   /* not implemented */
   (void)thread;
   return 0;
}

VCOS_INLINE_IMPL
void vcos_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity) {
   /* not implemented */
   vcos_unused(thread);
   vcos_unused(affinity);
}


VCOS_INLINE_IMPL
void vcos_thread_attr_setaffinity(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED affinity) {
   attrs->ta_affinity = affinity;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_settimeslice(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED ts) {
   attrs->ta_timeslice = ts;
}

VCOS_INLINE_IMPL
void _vcos_thread_attr_setlegacyapi(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED legacy) {
   attrs->legacy = legacy;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setautostart(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED autostart) {
   (void)attrs;
   (void)autostart;
}

VCOS_INLINE_IMPL
VCOS_LLTHREAD_T *vcos_llthread_current(void) {
   return (VCOS_LLTHREAD_T *)pthread_self();
}

/*
 * Mutexes
 */

#ifndef VCOS_USE_VCOS_FUTEX

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_create(VCOS_MUTEX_T *latch, const char *name) {
   int rc = pthread_mutex_init(latch, NULL);
   (void)name;
   if (rc == 0) return VCOS_SUCCESS;
   else return vcos_pthreads_map_errno();
}

VCOS_INLINE_IMPL
void vcos_mutex_delete(VCOS_MUTEX_T *latch) {
   int rc = pthread_mutex_destroy(latch);
   (void)rc;
   vcos_assert(rc==0);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_lock(VCOS_MUTEX_T *latch) {
   int rc = pthread_mutex_lock(latch);
   vcos_assert(rc==0);
   (void)rc;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_unlock(VCOS_MUTEX_T *latch) {
   int rc = pthread_mutex_unlock(latch);
   (void)rc;
   vcos_assert(rc==0);
}

VCOS_INLINE_IMPL
int vcos_mutex_is_locked(VCOS_MUTEX_T *m) {
   int rc = pthread_mutex_trylock(m);
   if (rc == 0) {
      pthread_mutex_unlock(m);
      /* it wasn't locked */
      return 0;
   }
   else {
      return 1; /* it was locked */
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_trylock(VCOS_MUTEX_T *m) {
   int rc = pthread_mutex_trylock(m);
   (void)rc;
   return (rc == 0) ? VCOS_SUCCESS : VCOS_EAGAIN;
}

#endif /* VCOS_USE_VCOS_FUTEX */

/*
 * Events
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_create(VCOS_EVENT_T *event, const char *debug_name)
{
   int rc = sem_init(event, 0, 0);
   (void)debug_name;
   if (rc != -1) return VCOS_SUCCESS;
   else return vcos_pthreads_map_errno();
}

VCOS_INLINE_IMPL
void vcos_event_signal(VCOS_EVENT_T *event)
{
   int rc = sem_post(event);
   vcos_assert(rc == 0);
   (void)rc;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_wait(VCOS_EVENT_T *event)
{
   int ret;
   /* gdb causes sem_wait() to EINTR when a breakpoint is hit, retry here */
   while ((ret = sem_wait(event)) == -1 && errno == EINTR)
      continue;
   vcos_assert(ret==0);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_try(VCOS_EVENT_T *event)
{
   int ret;
   while ((ret = sem_trywait(event)) == -1 && errno == EINTR)
      continue;

   if (ret == -1 && errno == EAGAIN)
      return VCOS_EAGAIN;
   else
      return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_event_delete(VCOS_EVENT_T *event)
{
   int rc = sem_destroy(event);
   vcos_assert(rc != -1);
   (void)rc;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_process_id_current(void) {
   return (VCOS_UNSIGNED) getpid();
}

VCOS_INLINE_IMPL
int vcos_strcasecmp(const char *s1, const char *s2) {
   return strcasecmp(s1,s2);
}

VCOS_INLINE_IMPL
int vcos_strncasecmp(const char *s1, const char *s2, size_t n) {
   return strncasecmp(s1,s2,n);
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void) {
   return 0;
}

/* For support event groups - per thread semaphore */
VCOS_INLINE_IMPL
void _vcos_thread_sem_wait(void) {
   VCOS_THREAD_T *t = vcos_thread_current();
   vcos_semaphore_wait(&t->suspend);
}

VCOS_INLINE_IMPL
void _vcos_thread_sem_post(VCOS_THREAD_T *target) {
   vcos_semaphore_post(&target->suspend);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_tls_create(VCOS_TLS_KEY_T *key) {
   int st = pthread_key_create(key, NULL);
   return st == 0 ? VCOS_SUCCESS: VCOS_ENOMEM;
}

VCOS_INLINE_IMPL
void vcos_tls_delete(VCOS_TLS_KEY_T tls) {
   pthread_key_delete(tls);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_tls_set(VCOS_TLS_KEY_T tls, void *v) {
   pthread_setspecific(tls, v);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void *vcos_tls_get(VCOS_TLS_KEY_T tls) {
   return pthread_getspecific(tls);
}

#if VCOS_HAVE_ATOMIC_FLAGS

/*
 * Atomic flags
 */

/* TODO implement properly... */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_atomic_flags_create(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
   atomic_flags->flags = 0;
   return vcos_mutex_create(&atomic_flags->mutex, "VCOS_ATOMIC_FLAGS_T");
}

VCOS_INLINE_IMPL
void vcos_atomic_flags_or(VCOS_ATOMIC_FLAGS_T *atomic_flags, uint32_t flags)
{
   vcos_mutex_lock(&atomic_flags->mutex);
   atomic_flags->flags |= flags;
   vcos_mutex_unlock(&atomic_flags->mutex);
}

VCOS_INLINE_IMPL
uint32_t vcos_atomic_flags_get_and_clear(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
   uint32_t flags;
   vcos_mutex_lock(&atomic_flags->mutex);
   flags = atomic_flags->flags;
   atomic_flags->flags = 0;
   vcos_mutex_unlock(&atomic_flags->mutex);
   return flags;
}

VCOS_INLINE_IMPL
void vcos_atomic_flags_delete(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
   vcos_mutex_delete(&atomic_flags->mutex);
}

#endif

#if defined(linux) || defined(_HAVE_SBRK)

/* not exactly the free memory, but a measure of it */

VCOS_INLINE_IMPL
unsigned long vcos_get_free_mem(void) {
   return (unsigned long)sbrk(0);
}

#endif

#ifdef VCOS_PTHREADS_WANT_HISR_EMULATION
VCOS_STATUS_T vcos_legacy_hisr_create(VCOS_HISR_T *hisr, const char *name,
                                      void (*entry)(void),
                                      VCOS_UNSIGNED pri,
                                      void *stack, VCOS_UNSIGNED stack_size);

void vcos_legacy_hisr_activate(VCOS_HISR_T *hisr);

void vcos_legacy_hisr_delete(VCOS_HISR_T *hisr);

#endif

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 0

#endif /* VCOS_INLINE_BODIES */

#define  vcos_log_platform_init()               _vcos_log_platform_init()
VCOSPRE_ void VCOSPOST_             _vcos_log_platform_init(void);

VCOS_INLINE_DECL void _vcos_thread_sem_wait(void);
VCOS_INLINE_DECL void _vcos_thread_sem_post(VCOS_THREAD_T *);

#define VCOS_APPLICATION_ARGC          vcos_get_argc()
#define VCOS_APPLICATION_ARGV          vcos_get_argv()

#include "interface/vcos/generic/vcos_generic_reentrant_mtx.h"
#include "interface/vcos/generic/vcos_generic_named_sem.h"
#include "interface/vcos/generic/vcos_generic_quickslow_mutex.h"
#include "interface/vcos/generic/vcos_common.h"

#define _VCOS_LOG_LEVEL() getenv("VC_LOGLEVEL")

VCOS_STATIC_INLINE
char *vcos_strdup(const char *str)
{
   return strdup(str);
}

typedef void (*VCOS_ISR_HANDLER_T)(VCOS_UNSIGNED vecnum);

#ifdef __cplusplus
}
#endif
#endif /* VCOS_PLATFORM_H */

