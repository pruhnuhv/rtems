/**
 * @file
 *
 * @ingroup POSIXAPI
 *
 * @brief Function Fetches State of POSIX Per-Process Timers
 */

/*
 *  14.2.4 Per-Process Timers, P1003.1b-1993, p. 267
 *
 *  COPYRIGHT (c) 1989-2008.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <errno.h>

#include <rtems/posix/timerimpl.h>
#include <rtems/score/todimpl.h>
#include <rtems/score/watchdogimpl.h>
#include <rtems/seterr.h>
#include <rtems/timespec.h>

/*
 *          - When a timer is initialized, the value of the time in
 *            that moment is stored.
 *          - When this function is called, it returns the difference
 *            between the current time and the initialization time.
 */

int timer_gettime(
  timer_t            timerid,
  struct itimerspec *value
)
{
  POSIX_Timer_Control *ptimer;
  ISR_lock_Context lock_context;
  Per_CPU_Control *cpu;
  struct timespec now;
  struct timespec expire;
  struct timespec result;

  if ( !value )
    rtems_set_errno_and_return_minus_one( EINVAL );

  ptimer = _POSIX_Timer_Get( timerid, &lock_context );
  if ( ptimer == NULL ) {
    rtems_set_errno_and_return_minus_one( EINVAL );
  }

  cpu = _POSIX_Timer_Acquire_critical( ptimer, &lock_context );
  rtems_timespec_from_ticks( ptimer->Timer.expire, &expire );

  if ( ptimer->clock_type == CLOCK_MONOTONIC ) {
  _Timecounter_Nanouptime(&now);
  } else  if (ptimer->clock_type == CLOCK_REALTIME) {
    _TOD_Get(&now);
  } else {
    _POSIX_Timer_Release( cpu, &lock_context );
    rtems_set_errno_and_return_minus_one( EINVAL );
  }


 if ( rtems_timespec_less_than( &now, &expire ) ) {
      rtems_timespec_subtract( &now, &expire, &result );
  } else {
    result.tv_nsec = 0;
    result.tv_sec  = 0;
  }

  value->it_value = result;
  value->it_interval = ptimer->timer_data.it_interval;

  _POSIX_Timer_Release( cpu, &lock_context );
  return 0;
}