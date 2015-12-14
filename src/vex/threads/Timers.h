/*
 * Timers.h: Class for the various timers that thread use
 *
 *  Created on: 21 Apr 2011
 *      Author: root
 */

#ifndef TIMERS_H_
#define TIMERS_H_

#include "MethodCallInfo.h"
#include "Time.h"

#define MAXIMUM_CONSECUTIVE_TIMESLICES_AS_BLOCKED_IN_MONITOR 10

/**
 * Stores and manipulates various related timers associated with a thread.
 */
class Timers {
  public:
    /**
     * Construct a new Timers and register with PAPI or perfctr.
     */
    Timers();
    virtual ~Timers();

    /**
     * Set #virtualTime to \p _virtualTime.
     */
    void setVirtualTime(const long long &_virtualTime) {
        virtualTime = _virtualTime;
    }

    /**
     * Returns #virtualTime.
     */
    long long getCurrentCpuTime() const {
        return virtualTime;
    }

    /**
     * Set #estimatedRealTime to \p _estimatedRealTime.
     */
    void setEstimatedRealTime(const long long &_estimatedRealTime) {
        estimatedRealTime = _estimatedRealTime;
    }

    /**
     * Add \p schedulerTimeslot to #estimatedRealTime.
     */
    void setSynchronizationBarrierTime(const long long &schedulerTimeslot) {
        estimatedRealTime += schedulerTimeslot;
    }

    /**
     * Subtract \p schedulerTimeslot from #estimatedRealTime.
     */
    void clearSynchronizationBarrierTime(const long long &schedulerTimeslot) {
        estimatedRealTime -= schedulerTimeslot;
    }

    /**
     * If \p currentTime is valid to leap to (larger than #estimatedRealTime),
     * then set #estimatedRealTime to it.
     */
    void leapForwardTo(const long long &currentTime) {
        if (estimatedRealTime < currentTime) {
            estimatedRealTime = currentTime;
        }
    }

    /**
     * Set #estimatedRealTime to \p currentVirtualTime.
     */
    void leapToGlobalTime(const long long &currentVirtualTime) {
        if (!dontUpdateToGlobalTime) {
            estimatedRealTime = currentVirtualTime;
        }
    }

    /**
     * Add external virtual time progress \p timeAdded to #virtualTime and
     * #estimatedRealTime.
     */
    void addGlobalTime(const long long &timeAdded) {
        addElapsedTime(timeAdded);
        localTimeSinceLastResume += timeAdded;
    }

    /**
     * Add internal virtual time progress \p timeDiff to #virtualTime and
     * #estimatedRealTime.
     */
    void addLocalTime(const long long &timeDiff) {
        // Take into account any time coefficients.
        long long timeAdded = timeDiff * virtualTimeSpeedup  * globalScalingFactor;
        addElapsedTime(timeAdded);
        localTimeSinceLastResume += timeAdded;
    }

    /**
     * Add I/O virtual time progress \p timeDiff to #estimatedRealTime and
     * #localTimeSinceLastResume but not CPU time #virtualTime.
     */
    void addLocalIoTime(const long long &timeDiff) {
        // Don't use globalScalingFactor here.
        long long timeAdded = timeDiff * virtualTimeSpeedup;
        estimatedRealTime 		 += timeAdded;
        localTimeSinceLastResume += timeAdded;
    }

    /**
     * Returns true if \p currentVirtualTime is larger than #estimatedRealTime.
     */
    bool beforeCurrentGlobalTime(const long long &currentVirtualTime) {
        if (currentVirtualTime > estimatedRealTime) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Returns #estimatedRealTime.
     */
    long long const& getEstimatedRealTime() const {
        return estimatedRealTime;
    }

    /**
     * Set #lastCPUTime to \p _lastCpuTime.
     */
    void setLastCPUTime(const long long &_lastCpuTime) {
        lastCPUTime = _lastCpuTime;
    }

    /**
     * Set #lastCPUTime to the current virtual time (user CPU time)
     */
    void updateCpuTimeClock() {
        lastCPUTime = getVirtualTime();
    }

    /**
     * Set #lastCPUTime to the current virtual time (user CPU time) plus
     * compensation for I/O instrumentation delay.
     */
    void setCpuTimeClockAfterIo() {
        lastCPUTime = getVirtualTime() + Time::getIoBytecodeDelay();
    }

    /**
     * Increase #lastCPUTime by the amount of time spent in a simulation. TODO
     */
    void updateCpuTimeAddingSimulatedVirtualTime() {
        lastCPUTime += (virtualTime - virtualTimeAtSimulationStart);
    }

    /**
     * Returns #lastCPUTime.
     */
    long long const& getLastCPUTime() const {
        return lastCPUTime;
    }

    /**
     * Set #lastRealTime to \p _lastRealTime.
     */
    void setLastRealTime(const long long & _lastRealTime) {
        lastRealTime = _lastRealTime;
    }

    /**
     * Returns #lastRealTime.
     */
    long long const & getLastRealTime() const {
        return lastRealTime;
    }

    /**
     * Set #localTimeSinceLastResume to \p newTime.
     */
    void setLocalTime(const long &newTime) {
        localTimeSinceLastResume = newTime;
    }

    /**
     * Returns #localTimeSinceLastResume.
     */
    long long const & getLocalTimeSinceLastResume() const {
        return localTimeSinceLastResume;
    }

    /**
     * Set #resumedLastAt to \p _resumedLastAt.
     */
    void setResumedLastAt(const long long &_resumedLastAt) {
        resumedLastAt = _resumedLastAt;
    }

    /**
     * Returns #resumedLastAt.
     */
    long long const& getResumedLastAt() const {
        return resumedLastAt;
    }

    /**
     * Set #resumedLastAtReal to \p _resumedLastAtReal.
     */
    void setResumedLastAtReal(const long long &_resumedLastAtReal) {
        resumedLastAtReal = _resumedLastAtReal;
    }

    /**
     * Set #resumedLastAtReal to the current real time (wallclock time).
     */
    void updateResumedLastRealAt() {
        resumedLastAtReal = Time::getRealTime();
    }

    /**
     * Returns #resumedLastAtReal.
     */
    long long const & getResumedLastAtReal() const {
        return resumedLastAtReal;
    }

    /**
     * Set #lastTimeInHandler to #estimatedRealTime and #lastRealTimeInHandler
     * to \p realHandlerTime.
     */
    void updateLastResumedTo(const long long &realHandlerTime) {
        lastTimeInHandler = estimatedRealTime;
        lastRealTimeInHandler = realHandlerTime;
    }

    /**
     * Returns #lastTimeInHandler.
     */
    long long const& getLastTimeInHandler() const {
        return lastTimeInHandler;
    }

    /**
     * Returns #lastRealTimeInHandler.
     */
    long long const& getLastRealTimeInHandler() const {
        return lastRealTimeInHandler;
    }

    /**
     * Update #resumedLastAt, #resumedLastAtReal, #lastTimeInHandler,
     * #lastRealTimeInHandler, and reset #localTimeSinceLastResume.
     */
    void updateTimesAfterResuming(const long long &resumingVirtualTime) {
        if (dontUpdateToGlobalTime) {
            // This is used for model-simulation remote timelines.
            resumedLastAt = estimatedRealTime;
        } else {
            resumedLastAt = resumingVirtualTime;
        }
        resumedLastAtReal 		= Time::getRealTime();
        lastTimeInHandler 		= resumedLastAt;
        lastRealTimeInHandler 	= resumedLastAtReal;
        localTimeSinceLastResume = 0;
    }

    /**
     * Set #lastTimePerStateERT to \p _lastTimePerStateERT.
     */
    void setLastTimePerStateERT(const long long &_lastTimePerStateERT) {
        lastTimePerStateERT = _lastTimePerStateERT;
    }

    /**
     * Returns #lastTimePerStateERT.
     */
    long long const & getLastTimePerStateERT() const {
        return lastTimePerStateERT;
    }

    /**
     * Save #virtualTime in #virtualTimeAtSimulationStart at the beginning of
     * a simulation. TODO
     */
    void logStartingVirtualTime() {
        virtualTimeAtSimulationStart = virtualTime;
    }

    /**
     * Set #ioWaitingTime.
     */
    void updateIoWaitingTimeFrom(const long long &startingWaiting, const long long &actualIoDuration) {
        long long leap = estimatedRealTime - startingWaiting;
        if (leap > 0) {
            if (leap > actualIoDuration) {
                ioWaitingTime += actualIoDuration;
            } else {
                ioWaitingTime += leap;
            }
        }
    }

    /**
     * Returns #ioWaitingTime.
     */
    long long const & getIoWaitingTime() const {
        return ioWaitingTime;
    };

    /**
     * If \p startingWaiting is smaller than #estimatedRealTime, add the
     * difference to #monitorWaitingTime.
     */
    void updateWaitingTimeFrom(const long long &startingWaiting) {
        long long leap = estimatedRealTime - startingWaiting;
        if (leap > 0) {
            monitorWaitingTime += leap;
        }
    }

    /**
     * Returns #monitorWaitingTime.
     */
    long long const & getMonitorWaitingTime() const {
        return monitorWaitingTime;
    }

    /**
     * Set #dontUpdateToGlobalTime to \p _dontUpdateToGlobalTime.
     */
    void setDontUpdateToGlobalTime(const bool &_dontUpdateToGlobalTime) {
        dontUpdateToGlobalTime = _dontUpdateToGlobalTime;
    }

    /**
     * Returns the value of #dontUpdateToGlobalTime.
     */
    bool shouldNotUpdateToGlobalTime() {
        return dontUpdateToGlobalTime;
    }

    /**
     * Set #virtualTimeSpeedup to \p factor.
     */
    void setTimeScalingFactor(const float &factor) {
        virtualTimeSpeedup = factor;
    }

    /**
     * Returns #virtualTimeSpeedup.
     */
    float const & getTimeScalingFactor() const {
        return virtualTimeSpeedup;
    }

    /**
     * If \p _globalScalingFactor is valid (positive real), then set
     * #globalScalingFactor to it.
     */
    static void setGlobalScalingFactor(const float &_globalScalingFactor) {
        if (_globalScalingFactor > 0.0) {
            globalScalingFactor = _globalScalingFactor;
        }
    }

    /**
     * Set #timeout to \p _timeout.
     */
    void setTimeout(const long &_timeout) {
        timeout = _timeout;
    }

    /**
     * Return #timeout.
     */
    long const& getTimeout() const {
        return timeout;
    }

    /**
     * Reset #timeout and #threadStillBlocked.
     */
    void setTimeoutFlagToDenotePossiblyResumingThread() {
        timeout = 0;
        threadStillBlocked = MAXIMUM_CONSECUTIVE_TIMESLICES_AS_BLOCKED_IN_MONITOR;
    }

    /**
     * Subtract #timeout from #estimatedRealTime and reset #timeout.
     */
    void rollbackFromTimeOutValue() {
        estimatedRealTime -= timeout;
        timeout = -1;
    }

    /**
     * Returns the current virtual time (user CPU time) in nanoseconds.
     *
     * Will try to use PAPI, clock_gettime() or perfctr in that order.
     *
     * PAPI timers use the most accurate timers available on the platform in
     * use. These timers can be used to obtain both _real_ and _virtual_ time
     * on each supported platform. The real time clock runs all the time (e.g.
     * a wall clock). The virtual time clock runs only when the processor is
     * running in user mode.
     *
     * clock_gettime(CLOCK_THREAD_CPUTIME_ID, struct timespec *) returns
     * thread-specific CPU-time clock, generally by using TSC on i386.
     *
     * PERFCTR_getVT_for() uses vperfctr_read_tsc() in libperfctr.h.
     * TODO Find more information on this library.
     */
    long long const getVirtualTime() const {
#if USE_PAPI == 1
        // PAPI_get_virt_nsec()
        // Get virtual time counter values in nanoseconds.
        // This function returns the total number of virtual units from some
        // arbitrary starting point. Virtual units accrue every time the
        // process is running in user-mode on behalf of the process.
        return PAPI_get_virt_nsec();
#elif USE_SYS_CLOCK == 1
        struct timespec currentRealTime;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &currentRealTime );
        return (long long)(currentRealTime.tv_sec * 1000000000 + currentRealTime.tv_nsec);
#else
        return PERFCTR_getVT_for(threadPerfPtr);
#endif
    }

    /**
     * Poll and return the current virtual time after compensating for
     * code instrumentation delays.
     */
    long long getThreadTimeBeforeMethodInstrumentation() {
        return getVirtualTime() - Time::getBytecodeDelay();
    }

    /**
     * Poll and return the current virtual time after compensating for I/O
     * instrumentation delays.
     */
    long long getThreadTimeBeforeIoMethodInstrumentation() {
        return getVirtualTime() - Time::getIoBytecodeDelay();
    }

    /**
     * TODO Returns the current virtual timestamp.
     */
    long long getCurrentVirtualTimestampOfRunningThread() {
        return resumedLastAt + localTimeSinceLastResume;
    }

    /**
     * Returns true if the difference between the current real (wallclock)
     * timestamp and the last cached value #lastRealTime is greater than 0.5
     * seconds.
     */
    bool maximumPossibleIoTimeExpired() {
        // The maximum I/O time is 0.5 seconds.
        return getCurrentTimeDifferenceFromLastUpdatedRealTime() > 500000000;
    }

    /**
     * TODO
     */
    void updateExitingMethodInfo(const int &methodId, MethodCallInfo *exitingMethod) {
        exitingMethod->setInfo(methodId, virtualTime, estimatedRealTime, monitorWaitingTime, ioWaitingTime);
    }

    /**
     * TODO Method used to support questions on virtual leaps forward in virtual
     * time.
     *
     * This method is only called when a thread wants to leap forward but
     * another thread is waiting on a monitor.
     *
     * Since we cannot be certain that the monitor has been released in the
     * meantime, we need to allow for some real time to pass
     * (MAXIMUM_CONSECUTIVE_TIMESLICES_AS_BLOCKED_IN_MONITOR *
     * SCHEDULER_TIMESLOT) before we denote that the thread is still blocked.
     * In that case the forward leaping thread is allowed to do so.
     */
    bool isThreadStillBlockedInMonitor() {
        if (timeout == 0) {
            // If a thread is asked and is found to be blocked for a
            // consecutive number of times, then release it
            return (threadStillBlocked-- > 0);
        } else {
            return false;
        }
    }

////////////////////////////////////////////////////////////////////////////////

    /**
     * TODO Using a heuristic based on the virtual time difference between the last
     * possible running thread (\p minimumMaybeAliveTime) and the real time
     * expired.
     */
    bool hasTimeoutExpiredInRealTime(const long long &minimumMaybeAliveTime, const long long &remainingTimeToExpireTimeout);

    /**
     * Set #timeout to \p _timeout then update #estimatedRealTime to when the
     * timeout would expire after taking into account time coefficients.
     *
     * Resets #threadStillBlocked if \p _timeout is 0.
     */
    void updateTimesBeforeWaitingWithTimeout(const long &_timeout);

    /**
     * If the current polled virtual time from getVirtualTime() is larger than
     * #lastCPUTime, then update #virtualTime and #estimatedRealTime.
     */
    void updateCurrentLocalTime();

    /**
     * Update #lastCPUTime to the current polled virual (user CPU) time from
     * getVirualTime() and #lastRealTime to the current polled real (wallclock)
     * time from Time#getRealTime().
     *
     * \returns Current virtual (user CPU) time
     */
    long long updateClocks();

    /**
     * Update #lastRealTime to the current polled real (wallclock) time from
     * Time#getRealTime().
     */
    void updateRealTimeClock();

    /**
     * Add a duration \p time to virtual time #virtualTime and estimated real
     * time #estimatedRealTime.
     *
     * Note that \p time is added literally. Any time coefficients such as
     * virtual time speed up factors or global scaling factors, must be taken
     * into consideration before calling this method.
     */
    long long addElapsedTime(long long time);

    /**
     * Add a duration \p time to #estimatedRealTime.
     */
    long long leapForwardBy(long long time);

    /**
     * Update #estimatedRealTime to \p time if possible, that is \p time is
     * larger.
     */
    long long leapTo(long long time);

    /**
     * Returns the difference between \p startingTime and #lastCPUTime after
     * time coefficients.
     */
    long long getTimeDifferenceUntil(const long long &startingTime);

    /**
     * Returns #localTimeSinceLastResume and reset its value to nil.
     */
    long long getAndResetLocalTime();

    /**
     * Returns #localTimeSinceLastResume.
     */
    long long getLocalTime();

    /**
     * If \p presetTime is larger than #lastCPUTime, that is, virtual time has
     * progressed, then update #virtualTime and #estimatedRealTime.
     */
    void updateThreadLocalTimeSinceLastResumeTo(const long long &presetTime);

    /**
     * If \p presetTime is larger than #lastRealTime, that is, virtual time has
     * progressed, then update #virtualTime and #estimatedRealTime.
     */
    void updateThreadLocalTimeSinceLastResumeToRealTime(const long long &presetTime);

    /**
     * TODO
     */
    long long getCurrentTimeDifferenceFromLastUpdatedRealTime();

    /**
     * TODO
     */
    long long getDifferenceBetweenCpuAndRealTimeSinceLastSignalling(const long long &realTime);

    /**
     * TODO
     */
    bool doesTheDifferenceBetweenCpuAndRealTimeIndicateLackOfProgress(const long long &realTime, const float &cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress);

    /**
     * TODO
     */
    long long getDifferenceBetweenCpuAndRealTime(const long long &realTime, const float &cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress);

  protected:
    /**
     * Called by constructor.
     */
    struct vperfctr *onThreadInit();

    /**
     * Called by destructor.
     */
    void onThreadEnd();

    /**
     * Virtual thread CPU time, (I believe this should be called
     * estimatedVirtualTime and is analogous to estimatedRealTime).
     */
    long long virtualTime;

    /**
     * I believe this is the estimated "virtual" time, that is, real wallclock
     * time after VEX-related time coefficients have been applied.
     *
     * For example, I/O time is only added to #estimatedRealTime and
     * #localTimeSinceLastResume, not #virtualTime.
     */
    long long estimatedRealTime;

    /**
     * Set and updated by getVirtualTime(), which polls the most recent virtual
     * time (thread user CPU time) using external performance/time libraries,
     * \p lastCPUTime is a local cached value.
     */
    long long lastCPUTime;

    /**
     * Set and updated by Time#getRealTime(), which polls the most recent real
     * time (wallclock time) using external performance/time libraries, \p
     * lastRealTime is analogous to #lastCPUTime and is a local cached value.
     */
    long long lastRealTime;

    /**
     * Virtual (thread CPU) time passed since the thread was last resumed.
     */
    long long localTimeSinceLastResume;

    /**
     * Virtual (thread CPU) timestamp of when the thread was last resumed.
     *
     * When the thread is resumed, \p resumedLastAt is set to
     * #estimatedRealTime (global) or #resumingVirtualTime (local?).
     */
    long long resumedLastAt;

    /**
     * Real (wallclock) timestamp of when the thread was last resumed.
     *
     * Set by updateResumedLastRealAt() by calling Time#getRealTime().
     */
    long long resumedLastAtReal;

    /**
     * TODO Related to #estimatedRealTime and #resumedLastAt.
     */
    long long lastTimeInHandler;

    /**
     * TODO Related to #resumedLastAtReal, set by updateLastResumedTo(),
        * possibly by calling Time#getRealTime().
     */
    long long lastRealTimeInHandler;

    /**
     * TODO
     */
    long long lastTimePerStateERT;

    /**
     * Virtual timestamp of when a simulation starts.
     */
    long long virtualTimeAtSimulationStart;

    /**
     * TODO
     */
    long long ioWaitingTime;

    /**
     * TODO
     */
    long long monitorWaitingTime;

    /**
     * If true, update #resumedLastAt to #estimatedRealTime in
     * updateTimesAfterResuming() instead of parameter \p resumingVirtualTime.
     */
    bool dontUpdateToGlobalTime;

    /**
     * Struct used when calling the perfctr library in getVirtualTime().
     */
    struct vperfctr *threadPerfPtr;

    /**
     * Thread local virtual time scaling factor.
     */
    float virtualTimeSpeedup;

    /**
     * Global virtual time scaling factor.
     */
    static float globalScalingFactor;

    /**
     * Changes estimatedRealTime.
     */
    long timeout;

    /**
     * Denotes whether a thread is still blocked at a monitor, used in
     * heuristic for avoiding leaps forward.
     */
    int threadStillBlocked;

    /**
     * Reset all time variables in this Timers.
     */
    void resetCounters();
};

#endif /* TIMERS_H_ */
