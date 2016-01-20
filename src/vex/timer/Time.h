/*
 * Time.h: Class to return real/virtual timestamps
 *
 *  Created on: 1 Sep 2011
 *      Author: root
 */

#ifndef TIME_H_
#define TIME_H_

#include <ostream>

#ifndef USE_PAPI
# ifndef USE_PERFCTR
#  define USE_SYS_CLOCK 1
# else
#  define USE_PERFCTR 1
# endif
#else
# define USE_PAPI 1
#endif

#ifdef USE_PAPI
# include <papi.h>
#elif USE_SYS_CLOCK == 1
# include <time.h>
# include <unistd.h>
# include <sys/time.h>
#elif USE_PERFCTR == 1
extern "C" {
# include "perfdirect.h"
}
#endif

/**
 * XXX Retrieves real and virtual timestamps.
 */
class Time
{
 public:
  /**
   * Called when the Time object is initialized, initialize the underlying
   * timing library.
   */
  static bool onSystemInit();

  /**
   * Called when the Time object is closed, finalizes the underlying timing
   * library.
   */
  static void onSystemClose();

  /**
   * Additional code that is called when a thread is initialized.
   */
  static void onThreadInit();

  /**
   * Return true if PAPI is the underlying timing library.
   */
  static bool usingPapiTimer();

  /**
   * Return the current virtual time (user CPU time) in nanoseconds.
   *
   * Will try to use PAPI, clock_gettime() or perfctr in that order.
   *
   * This is almost identical to Timers#getVirtualTime, and is not actually
   * called.
   */
  static long long getVirtualTime() {
#if USE_PAPI == 1
    return PAPI_get_virt_nsec();
#elif USE_SYS_CLOCK == 1
    struct timespec currentCpuTime;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &currentCpuTime);
    return (long long)(currentCpuTime.tv_sec * 1000000000 + currentCpuTime.tv_nsec);
#elif USE_PERFCTR == 1
    return PERFCTR_getVT();
#endif
  }

  /**
   * Return the current real time (wallclock time) in nanoseconds.
   *
   * Will try to use PAPI, clock_gettime() or perfctr in that order.
   */
  static long long getRealTime() {
#if USE_PAPI == 1
    return PAPI_get_real_nsec();
#elif USE_SYS_CLOCK == 1
    struct timespec currentRealTime;
    clock_gettime(CLOCK_REALTIME, &currentRealTime );
    return (long long)(currentRealTime.tv_sec * 1000000000 + currentRealTime.tv_nsec);
#elif USE_PERFCTR == 1
    return PERFCTR_getRT();
#endif
  }

  /**
   * Return #bytecodeDelay.
   */
  static long long getBytecodeDelay() {
    return bytecodeDelay;
  }

  /**
   * Return #ioBytecodeDelay.
   */
  static long long getIoBytecodeDelay() {
    return ioBytecodeDelay;
  }

  /**
   * Return the current real time minus #ioBytecodeDelay in nanoseconds.
   */
  static long long getRealTimeBeforeMethodInstrumentation() {
    return getRealTime() - ioBytecodeDelay;
  }

  /**
   * Return the current virtual time minus #interactionPointDelay in
   * nanoseconds.
   */
  static long long getThreadTimeBeforeInteractionPoint() {
    return getVirtualTime() - interactionPointDelay;
  }

  /**
   * Write the current values of #bytecodeDelay, #interactionPointDelay,
   * #ioBytecodeDelay and #yieldDuration to a file given by \p filename.
   */
  static void writeDelaysToFile(const char *filename);

  /**
   * Attempt to parse a file \p filename containing #bytecodeDelay,
   * #interactionPointDelay, #ioBytecodeDelay and #yieldDuration then return
   * true; otherwise set all delays to zero and return true.
   */
  static bool parseDelaysFile(const char *filename);

  /**
   * Increment #bytecodeDelay, #ioBytecodeDelay and #interactionPointDelay by
   * \p methodDelay, \p ioMethod and \p ipDelay respectively.
   */
  static void setDelays(long methodDelay, long ioMethod, long ipDelay) {
    bytecodeDelay += methodDelay;
    ioBytecodeDelay += ioMethod;
    interactionPointDelay += ipDelay;
  }

  /**
   * Profile the average amount of virtual (CPU) and real (wallclock) time each
   * #getVirtualTime and #getRealTime takes over 10 million iterations.
   */
  static void profileTimers();

  /**
   * Profile the duration of sched_yield over 10,000 iterations.
   */
  static int measureYieldDuration();

  /**
   * Profile the cost per virtual time measurement (calling #getVirtualTime)
   * over 1000 iterations.
   */
  static long long profileDelayPerMeasurement();

  /**
   * Print #bytecodeDelay or #methodDelay, and #interactionPointDelay to
   * \p stderr.
   */
  static void printDelays();

  /**
   * Print #bytecodeDelay, #interactionPointDelay, #ioBytecodeDelay,
   * #yieldDuration, #totalBackgroundLoadExecutionTime, #totalCollections and
   * the mean of #totalBackgroundLoadExecution Time to \p myfile.
   */
  static void printDelaysToFile(std::ofstream &myfile);

  /**
   * Precisely print \p timeValue to \p out.
   *
   * \p htmlEncoding is used to determine how to print the "mu" character.
   */
  static void concisePrinting(std::ostream &out, const long long &timeValue, bool htmlEncoding);

  // The next calls are used to measure the duration of background load
  // executing in real time - though generic as this can be the motivation was
  // to trap GC in Java
  /**
   * Set #lastBackgroundLoadExecutionTime to the current real time.
   */
  static void onBackgroundLoadExecutionStart();

  /**
   * Return the time difference between the current real time and
   * #lastBackgroundLoadExecutionTime, then add this to
   * #totalBackgroundLoadExecutionTime and increment #totalCollections.
   */
  static long long getBackgroundLoadExecutionRealDuration();

  /**
   * Return #yieldDuration.
   */
  static int getYieldDuration() {
    return yieldDuration;
  }

protected:
  /**
   * Profiled instrumentation delay, set by the upper language-dependent VEX.
   *
   * For the reference implementation JINE, this is set by
   * InstrumentationProfiler.
   */
  static int bytecodeDelay;

  /**
   * Profiled I/O instrumentation delay, set by the upper language-dependent
   * VEX.
   *
   * For the reference implementation JINE, this is set by
   * InstrumentationProfiler.
   */
  static int ioBytecodeDelay;

  /**
   * Profiled interaction point delay, set by the upper language-dependent VEX.
   *
   * For the reference implementation JINE, this is set by
   * InstrumentationProfiler.
   */
  static int interactionPointDelay;

  /**
   * Profiled median duration of sched_yield() calls, set by
   * #measureYieldDuration.
   */
  static int yieldDuration;

  /**
   * Stores the real time when background load execution starts (i.e. when
   * #onBackgroundLoadExecutionStart is called).
   */
  static long long lastBackgroundLoadExecutionTime;

  /**
   * Set by #getBackgroundLoadExecutionRealDuration, and stores the total amount
   * of real time spent executing background tasks.
   */
  static long long totalBackgroundLoadExecutionTime;

  /**
   * Set by #getBackgroundLoadExecutionRealDuration, and stores the total number
   * of times the background load has been timed.
   */
  static long long totalCollections;
};

#endif /* TIME_H_ */
