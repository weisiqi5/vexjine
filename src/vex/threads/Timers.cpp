/*
 * Timers.cpp
 *
 *  Created on: 21 Apr 2011
 *      Author: root
 */

#include "Timers.h"

#include <cmath>
#include <climits>

using namespace std;

float Timers::globalScalingFactor = 1.0;

Timers::Timers() {
  resetCounters();
  lastCPUTime = 0;
  // The thread CPU-time measurement starts from this point.
  threadPerfPtr = onThreadInit();
}

struct vperfctr * Timers::onThreadInit() {
#if USE_PAPI == 1
  PAPI_register_thread();
  return NULL;
#elif USE_SYS_CLOCK == 1
  return NULL;
#elif USE_PERFCTR == 1
  return PERFCTR_register_thread();
#endif
}

Timers::~Timers() {
  threadPerfPtr = NULL;
  onThreadEnd();
}

void Timers::onThreadEnd() {
#if USE_PAPI == 1
  PAPI_unregister_thread();
#elif USE_PERFCTR == 1
  PERFCTR_unregister_thread();
#endif
}

bool Timers::hasTimeoutExpiredInRealTime(const long long &minimumMaybeAliveTime, const long long &remainingTimeToExpireTimeout) {
  if (timeout <= 0) {
    return true;
  }

  long long difference = estimatedRealTime - minimumMaybeAliveTime;

  if (difference < 0) {
    // minimumMaybeAliveTime is after estimatedRealTime
    return true;
  } else {
    double factor = 1.0;

    // TODO Hack to avoid huge leaps.
    if (difference > 100000000) {
      factor = 100 * (double) log10(difference) - 7.0;
    }

    if (getCurrentTimeDifferenceFromLastUpdatedRealTime() > (double)(factor*remainingTimeToExpireTimeout)) {
      return true;
    } else {
      return false;
    }
  }
}

void Timers::updateTimesBeforeWaitingWithTimeout(const long &_timeout) {
  timeout = _timeout;
  if (_timeout == 0) {
    threadStillBlocked = MAXIMUM_CONSECUTIVE_TIMESLICES_AS_BLOCKED_IN_MONITOR;
  } else {
    if (_timeout >= (LONG_MAX - estimatedRealTime)/(virtualTimeSpeedup * globalScalingFactor)) {
      estimatedRealTime = LONG_MAX;
    } else {
      estimatedRealTime += (timeout * virtualTimeSpeedup * globalScalingFactor);
    }
  }
}

void Timers::updateCurrentLocalTime() {
  updateThreadLocalTimeSinceLastResumeTo(getVirtualTime());
}

long long Timers::updateClocks() {
  lastCPUTime = getVirtualTime();
  lastRealTime = Time::getRealTime();
  return lastCPUTime;
}

void Timers::updateRealTimeClock() {
  lastRealTime = Time::getRealTime();
}

long long Timers::addElapsedTime(long long time) {
  virtualTime         += time;
  estimatedRealTime     += time;
  return virtualTime;
}

long long Timers::leapForwardBy(long long time) {
  estimatedRealTime += time;
  return estimatedRealTime;
}

long long Timers::leapTo(long long time) {
  if (estimatedRealTime < time) {
    estimatedRealTime = time;
  }
  return estimatedRealTime;
}

long long Timers::getTimeDifferenceUntil(const long long &startingTime) {
  return (startingTime - lastCPUTime) * virtualTimeSpeedup * globalScalingFactor + getAndResetLocalTime();
}

long long Timers::getAndResetLocalTime() {
  long long temp = localTimeSinceLastResume;
  localTimeSinceLastResume = 0;
  return temp;
}

long long Timers::getLocalTime() {
  return localTimeSinceLastResume;
}

void Timers::updateThreadLocalTimeSinceLastResumeTo(const long long &presetTime) {
  long long timeDiff = presetTime - lastCPUTime;
  if (timeDiff > 0) {
    addLocalTime(timeDiff);
  }
}

void Timers::updateThreadLocalTimeSinceLastResumeToRealTime(const long long &presetTime) {
  long long timeDiff = presetTime - lastRealTime;
  if (timeDiff > 0) {
    addLocalTime(timeDiff);
  }
}

long long Timers::getCurrentTimeDifferenceFromLastUpdatedRealTime() {
  return Time::getRealTime() - lastRealTime;
}

long long Timers::getDifferenceBetweenCpuAndRealTimeSinceLastSignalling(const long long &realTime) {
  long long cpuTimeDifferenceSinceLastResume = (estimatedRealTime - lastTimeInHandler)/(virtualTimeSpeedup * globalScalingFactor); //inaccurate
  long long realTimeDifferenceSinceLastResume = realTime - lastRealTimeInHandler;
  return realTimeDifferenceSinceLastResume - cpuTimeDifferenceSinceLastResume;
}

bool Timers::doesTheDifferenceBetweenCpuAndRealTimeIndicateLackOfProgress(const long long &realTime, const float &cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress) {
  long long cpuTimeDifferenceSinceLastResume = (estimatedRealTime - lastTimeInHandler)/(virtualTimeSpeedup * globalScalingFactor); //inaccurate
  long long realTimeDifferenceSinceLastResume = realTime - lastRealTimeInHandler;
  return cpuTimeDifferenceSinceLastResume < (cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress * realTimeDifferenceSinceLastResume);
}

long long Timers::getDifferenceBetweenCpuAndRealTime(const long long &realTime, const float &cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress) {
  long long cpuTimeDifferenceSinceLastResume = (estimatedRealTime - lastTimeInHandler)/(virtualTimeSpeedup * globalScalingFactor); //inaccurate
  long long realTimeDifferenceSinceLastResume = realTime - lastRealTimeInHandler;
  return cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress * realTimeDifferenceSinceLastResume - cpuTimeDifferenceSinceLastResume;
}

void Timers::resetCounters() {
  timeout = -1;            // so that the scheduler will disregard it

  virtualTime = 0;

  resumedLastAt        = 0;
  resumedLastAtReal     = 0;

  estimatedRealTime     = 0;
  virtualTimeSpeedup     = 1.0;            // is one and multiplicated/divided on methodEnter/methodExit

  lastRealTime         = Time::getRealTime();
  resumedLastAtReal     = lastRealTime;

  lastTimePerStateERT      = 0;
  localTimeSinceLastResume = 0;

  lastTimeInHandler         = estimatedRealTime;    // used for the native waiting heuristics
  lastRealTimeInHandler     = lastRealTime;

  dontUpdateToGlobalTime  = false;

  ioWaitingTime         = 0;
  monitorWaitingTime     = 0;
}
