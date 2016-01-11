/*
 * VirtualTimeline.h: timeline file
 *
 *  Created on: 28 Jun 2010
 *      Author: nb605
 */

#ifndef VIRTUALTIMELINE_H_
#define VIRTUALTIMELINE_H_

#include <iostream>
#include <fstream>

class VexThreadState;
struct vex_and_system_states;
class TimeLoggingBehaviour;

/**
 * XXX Represents a virtual timeline.
 */
class VirtualTimeline
{
  public:
    VirtualTimeline();
    virtual ~VirtualTimeline();

    /**
     * Returns the current global virtual time.
     *
     * Any non-manager lock should only be acquired by a thread, after the
     * thread has acquired its controlling manager lock, so that it will not get
     * signalled, while holding the lock essentially blocking its controller =>
     * deadlock. The getGlobalTime() is therefore used to avoid acquiring two
     * locks in cases where the exact value of getGlobalTime does not matter.
     */
    long long const & getGlobalTime() {
        return globalVirtualTime;
    }

    /**
     * Reset this virtual timeline to the beginning of time.
     */
    virtual void reset();

    /**
     * If possible, update the global virtual time to \p forwardTime.
     *
     * Since the unknownParallelTime has already been added to the GVT
     * we have to include it to the first thread that will try to commit its
     * time after the unknownParallelTime has been increased.
     * Had we not done this, then the forwardTime (which is calculated as the
     * virtual time progress from lastResumed + localTime = forwardTime)
     * would overlap with the unknownParallelTime (= lastIdentifiedAsNw +
     * rndCpuDuration).
     */
    virtual void leapForwardTo(const long long &forwardTime);

    /**
     * Increase the global virtual time by \p forwardTime from non-VEX threads,
     * for example time spent by the GC.
     */
    virtual void addTimeOfUnknownRealTimeDurationAndSynchronize(const long long &forwardTime);

    /**
     * Increase the global virtual time by \p forwardTime from VEX threads that
     * has been permitted to run loosely due to native waiting.
     *
     * The difference from the addTimeOfUnknownRealTimeDurationAndSynchronize
     * method is that this time is only to be added to a (here THE) single
     * processor, while in the other case it should be added to all cores that
     * would have to synchronise after that as well.
     */
    virtual void addTimeOfThreadExecutingAtUnknownTime(const long long &forwardTime);

protected:
    pthread_mutex_t mutex;

    /**
     * Current global virtual time.
     */
    long long globalVirtualTime;

    /**
     * Time attributed to events not scheduled by VEX, for example native
     * waiting threads.
     */
    long long unknownParallelTime;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * XXX Represents a virtual timeline for multicore processors.
 *
 * There is a single global virtual time and a local virtual time for each
 * logical processor.
 */
class MulticoreVirtualTimeline : public VirtualTimeline
{
  public:
    /**
     * Construct a new multicore virtual timeline for \p schedulers logical
     * processors.
     */
    MulticoreVirtualTimeline(const int &schedulers);
    virtual ~MulticoreVirtualTimeline();

    /**
     * Returns the current local virtual time for logical processor
     * \p managerId.
     */
    long long const & getLocalTime(const int &managerId) {
        return localCoreVirtualTimes[managerId];
    }

    /**
     * Reset this virtual timeline to the beginning of time.
     */
    virtual void reset();

    /**
     * Update the global virtual time to \p time.
     */
    virtual void updateGlobalTimeTo(const long long &time);

    /**
     * Update the local virtual time for logical processor \p localTimelineId
     * (if possible) to \p time, and the global virtual time to the local
     * virtual time for logical processor \p localTimelineId whether updated or
     * not.
     */
    void leapForwardTo(const long long &time, const int &localTimelineId);

    /**
     * Increase the global virtual time by \p forwardTime from non-VEX threads,
     * for example time spent by the GC.
     *
     * Adds \p forwardTime to the latest local virtual time, then updates all
     * local virtual times and the global virtual time to this new virtual time.
     */
    void addTimeOfUnknownRealTimeDurationAndSynchronize(const long long &forwardTime);

    /**
     * Increase the global virtual time by \p forwardTime from VEX threads that
     * has been permitted to run loosely due to native waiting.
     *
     * Adds \p forwardTime to the oldest local virtual time, then updates the
     * global virtual time.
     *
     * It makes (some) sense that the native waiting thread was executed on the
     * logical processor with the least amount of load; even better if it was
     * idle.
     */
    void addTimeOfThreadExecutingAtUnknownTime(const long long &forwardTime);

    /**
     * Returns true if the local virtual time for logical processor \p
     * localTimelineId is more than one \p schedulerTimeslot timeslice ahead of
     * the global virtual time AND at least one logical processor is enabled.
     */
    bool shouldBlockCoreProgress(const long long &schedulerTimeslot, const int &localTimelineId);

    /**
     * DO NOT USE Returns false.
     */
    virtual bool disableLocalTimeline(const int &id);

    /**
     * Update the local virtual time for logical processor \p
     * id (if possible) and the global virtual time to \p threadTime.
     *
     * \returns The local virtual time for logical processor \p id, whether
     * updated or not.
     */
    virtual long long const & getLocalTimelineActivationTime(const long long &threadTime, const int &id);

    /**
     * Returns the current global virtual time.
     */
    virtual long long const & getUpdatedGlobalTime();

protected:
    /**
     * Array of local virtual times, one for each logical processor.
     */
    long long *localCoreVirtualTimes;

    /**
     * Number of logical processors.
     */
    int processors;
};

////////////////////////////////////////////////////////////////////////////////

/***
 * XXX Represents a strict virtual timeline for multicore processors.
 *
 * There is a single global virtual time for all logical processors.
 */
class DisablingMulticoreVirtualTimeline : public MulticoreVirtualTimeline
{
  public:
    /**
     * Construct a new multicore virtual timeline for \p schedulers logical
     * processors.
     */
    DisablingMulticoreVirtualTimeline(const int &schedulers);
    ~DisablingMulticoreVirtualTimeline();

    /**
     * Reset this virtual timeline to the beginning of time.
     */
    void reset();

    /**
     * Update the global virtual time, which should be the oldest local virtual
     * time.
     *
     * \param time is unused.
     *
     * Not synchronized.
     */
    void updateGlobalTimeTo(const long long &time);

    /**
     * Disable logical processor \p id and returns true if all logical
     * processors are inactive.
     */
    bool disableLocalTimeline(const int &id);

    /**
     * Enable logical processor \p id and update both its local virtual time (if
     * possible) and the global virtual time, which should be the oldest local
     * virtual time.
     *
     * \returns The local virtual time for logical processor \p id, whether
     * updated or not.
     */
    long long const & getLocalTimelineActivationTime(const long long &threadTime, const int &id);

    /**
     * Update the global virtual time, which should be the oldest local virtual
     * time.
     *
     * \returns The updated global virtual time.
     */
    long long & getUpdatedGlobalTime();

protected:
    /**
     * Array of whether a logical processor is enabled or not.
     */
    bool *localCoreActive;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * TODO
 */
class VirtualTimeSnapshot
{
  public:
    VirtualTimeSnapshot() {
        globalTime = 0;
        localTimelineId = 0;
        localTime = 0;
    }

    VirtualTimeSnapshot & operator=(const long long &_globalTime) {
        globalTime = _globalTime;
        localTimelineId = 0;
        localTime = _globalTime;
        return *this;
    }

    void set(const long long &_globalTime, const long long &_localTime, const int &_localTimelineId) {
        globalTime = _globalTime;
        localTime = _localTime;
        localTimelineId = _localTimelineId;
    }

    long long const &getGlobalTime() const {
        return globalTime;
    }

    long long const &getLocalTime() const {
        return localTime;
    }

    int const &getLocalTimelineId() const {
        return localTimelineId;
    }

    VirtualTimeSnapshot & operator-=(const VirtualTimeSnapshot &rhs);

  private:
    long long globalTime;
    int localTimelineId;
    long long localTime;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * XXX Interface for manipulating a VirtualTimeline using a Timers associated with
 * a VexThreadState.
 */
class VirtualTimelineController
{
  public:
    virtual void commitCpuTimeProgress(VexThreadState *state) = 0;
    virtual void commitIoTimeProgress(VexThreadState *state, const long long &actualIoDuration) = 0;
    virtual void commitTimedWaitingProgress(VexThreadState *state) = 0;
    virtual void commitNativeWaitingProgress(VexThreadState *state) = 0;
    virtual void commitModelSimulationProgress(VexThreadState *state) = 0;
    virtual void commitTimedOutIoProgress(VexThreadState *state) = 0;
    virtual void commitBackgroundLoadExecutionTime(const long long &backgroundLoadExecutionTime) = 0;

    virtual void updateResumingSuspendedThreadTimestamp(VexThreadState *state) = 0;
    virtual void updateResumingSuspendedThreadResumedLastTimestamp(VexThreadState *state) = 0;
    virtual void updateTimedOutWaitingThreadTimestamp(VexThreadState *state) = 0;
    virtual void updateInterruptedWaitingThreadTimestamp(VexThreadState *state, const long long &interruptedTime) = 0;
    virtual void updateBlockedThreadTimestamp(VexThreadState *state) = 0;

    virtual long long getHowFarAheadInVirtualTimeTheThreadIs(VexThreadState *state) = 0;
    virtual bool shouldBlockCoreProgress(const long long &schedulerTimeslot) = 0;
    virtual bool disableCore() = 0;
    virtual long long const & getGlobalTime() = 0;

    virtual void getCurrentTimeSnapshot(VirtualTimeSnapshot &vts) = 0;
    // TIME ADDITION STATISTICS - THIS IS THE MOST IMPORTANT BIT FOR ONCE AND FORALL FIXES
    // void addStatsSample(const int &position, const long long &time) {
    //     timestats.addSample(position, time);
    // };

    // OUTPUT METHODS
    // void printStats(char *filename);
};

////////////////////////////////////////////////////////////////////////////////

/**
 * XXX Basic implementation of VirtualTimelineController, responsible for a single
 * VirtualTimeline.
 */
class SingleVirtualTimelineController : public VirtualTimelineController
{
  public:
    /**
     * Create a new controller that is responsible for virtual timeline \p
     * _timeline.
     */
    SingleVirtualTimelineController(VirtualTimeline *_timeline);

    /**
     * Try to update the global virtual time to the current virtual time in
     * thread \p state.
     */
    virtual void commitCpuTimeProgress(VexThreadState *state);

    /**
     * Update Timers#ioWaitingTime in thread \p state using the current global
     * virtual time, then try to update the global virtual time to
     * Timers#estimatedRealTime in thread \p state.
     */
    virtual void commitIoTimeProgress(VexThreadState *state, const long long &actualIoDuration);

    /**
     * Update Timers#monitorWaitingTime in thread \p state using the current
     * global virtual time, then try to update the global virtual time to
     * Timers#estimatedRealTime in thread \p state.
     */
    void commitTimedWaitingProgress(VexThreadState *state);

    /**
     * Increase the global virtual time by Timers#localTimeSinceLastResume in
     * thread \p state.
     */
    void commitNativeWaitingProgress(VexThreadState *state);

    /**
     * Increase the global virtual time by \p backgroundLoadExecutionTime.
     */
    void commitBackgroundLoadExecutionTime(const long long &backgroundLoadExecutionTime);

    /**
     * If the thread \p state is not currently waiting in a model simulation,
     * try to update the global virtual time to the current virtual time in
     * thread \p state.
     */
    void commitModelSimulationProgress(VexThreadState *state);

    /**
     * DO NOT USE Try to update the global virtual time to
     * Timers#estimatedRealTime in thread \p state.
     */
    void commitTimedOutIoProgress(VexThreadState *state);

    /**
     * Update Timers#estimatedRealTime in thread \p state to the current global
     * virtual time.
     */
    void updateResumingSuspendedThreadTimestamp(VexThreadState *state);

    /**
     * Update Timers#resumedLastAt in thread \p state to the current global
     * virtual time.
     */
    void updateResumingSuspendedThreadResumedLastTimestamp(VexThreadState *state);

    /**
     * Update Timers#estimatedRealTime in thread \p state to the current global
     * virtual time.
     */
    void updateTimedOutWaitingThreadTimestamp(VexThreadState *state);

    /**
     * Update Timers#estimatedRealTime in thread \p state to \p interruptedTime
     * if it is non-zero, otherwise update to the current global virtual time.
     */
    void updateInterruptedWaitingThreadTimestamp(VexThreadState *state, const long long &interruptedTime);

    /**
     * Update Timers#estimatedRealTime to the current global virtual time, set
     * Timers#resumedLastAt to the current global virtual time and reset
     * Timers#localTimeSinceLastResume in thread \p state.
     */
    void updateBlockedThreadTimestamp(VexThreadState *state);

    /**
     * Returns the difference between the current global virtual time and the
     * Timers#estimatedRealTime in thread \p state.
     */
    long long getHowFarAheadInVirtualTimeTheThreadIs(VexThreadState *state);

    /**
     * DO NOT USE Returns true.
     */
    bool shouldBlockCoreProgress(const long long &schedulerTimeslot);

    /**
     * DO NOT USE Returns false.
     */
    bool disableCore();

    /**
     * Returns the current global virtual time.
     */
    long long const & getGlobalTime();

    /**
     * Set the virtual time snapshot \p vts to the current global virtual time.
     */
    void getCurrentTimeSnapshot(VirtualTimeSnapshot &vts);

protected:
    /**
     * The VirtualTimeline under control by this VirtualTimelineController.
     */
    VirtualTimeline *virtualTimeline;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * XXX The same as SingleVirtualTimelineController, except only used when the VEX
 * scheduler is not used.
 */
class PassiveVirtualTimelineController : public SingleVirtualTimelineController
{
  public:
    PassiveVirtualTimelineController(VirtualTimeline *_timeline);

    /**
     * Increase the global virtual time by Timers#localTimeSinceLastResume
     * in thread \p state, then set Timers#estimatedRealTime in thread \p state
     * to the current global virtual time.
     */
    void commitCpuTimeProgress(VexThreadState *state);

    /**
     * Update Timers#ioWaitingTime in thread \p state to the current global
     * virtual time, increase the global virtual time by
     * Timers#localTimeSinceLastResume in thread \p state, then set
     * Timers#estimatedRealTime in thread \p state to the current global virtual
     * time.
     */
    void commitIoTimeProgress(VexThreadState *state, const long long &actualIoDuration);
};

////////////////////////////////////////////////////////////////////////////////

/**
 * XXX Implementation of VirtualTimelineController, responsible for a single logical
 * processor #localTimelineId in a MulticoreVirtualTimeline.
 */
class MultipleVirtualTimelinesController : public VirtualTimelineController
{
  public:
    /**
     *
     */
    MultipleVirtualTimelinesController(MulticoreVirtualTimeline *_timelines, const int &_managerId);

    /**
     * Try to update the assigned logical processor's local virtual time to the
     * the current virtual time in thread \p state.
     */
    void commitCpuTimeProgress(VexThreadState *state);

    /**
     * Update Timers#ioWaitingTime in thread \p state using the assigned logical
     * processor's local virtual time, then try to update the assigned logical
     * processor's local virtual time to Timers#estimatedRealTime in thread \p
     * state.
     */
    void commitIoTimeProgress(VexThreadState *state, const long long &actualIoDuration);

    /**
     * Update Timers#monitorWaitingTime in thread \p state using the assigned
     * logical processor's local virtual time, then try to update the assigned
     * logical processor's local virtual time to Timers#estimatedRealTime in
     * thread \p state.
     */
    void commitTimedWaitingProgress(VexThreadState *state);

    /**
     * Increase the global virtual time by Timers#localTimeSinceLastResume in
     * thread \p state.
     */
    void commitNativeWaitingProgress(VexThreadState *state);

    /**
     * Increase the global virtual time by \p backgroundLoadExecutionTime.
     */
    void commitBackgroundLoadExecutionTime(const long long &backgroundLoadExecutionTime);

    /**
     * If the thread \p state is not currently waiting in a model simulation,
     * try to update the assigned logical processor's local virtual time and
     * Timers#estimatedRealTime in thread \p state to the current virtual time
     * in thread \p state.
     */
    void commitModelSimulationProgress(VexThreadState *state);

    /**
     * DO NOT USE Try to update the assigned logical processor's local virtual
     * time to Timers#estimatedRealTime in thread \p state.
     */
    void commitTimedOutIoProgress(VexThreadState *state);

    /**
     * Update Timers#estimatedRealTime in thread \p state to the assigned
     * logical processor's local virtual time, and synchronize the assigned
     * logical processor's local virtual time with Timers#estimatedRealTime.
     */
    void updateResumingSuspendedThreadTimestamp(VexThreadState *state);

    /**
     * Update the assigned logical processor's local virtual time to
     * Timers#estimatedRealTime if possible, then synchronize
     * Timers#estimatedRealTime and Timers#resumedLastAt with the assigned
     * logical processor's local virtual time.
     */
    void updateResumingSuspendedThreadResumedLastTimestamp(VexThreadState *state);

    /**
     * Update Timers#estimatedRealTime in thread \p state to \p interruptedTime
     * if it is non-zero, otherwise update to the assigned logical processor's
     * local virtual time.
     */
    void updateInterruptedWaitingThreadTimestamp(VexThreadState *state, const long long &interruptedTime);

    /**
     * Update Timers#estimatedRealTime in thread \p state to the assigned
     * logical processor's local virtual time.
     */
    void updateTimedOutWaitingThreadTimestamp(VexThreadState *state);

    /**
     * Update Timers#estimatedRealTime and Timers#resumedLastAt in thread \p
     * state to the current global virtual time, then reset
     * Timers#localTimeSinceLastResume.
     */
    void updateBlockedThreadTimestamp(VexThreadState *state);

    /**
     * Returns the difference between Timers#estimatedRealTime in thread \p
     * state and the current global time.
     */
    long long getHowFarAheadInVirtualTimeTheThreadIs(VexThreadState *state);

    /**
     * Returns true if the assigned logical processor's local virtual time is
     * more than one \p schedulerTimeslot timeslice ahead of the global virtual
     * time AND at least one logical processor is enabled.
     */
    bool shouldBlockCoreProgress(const long long &schedulerTimeslot);

    /**
     * DO NOT USE Returns false.
     */
    bool disableCore();

    /**
     * Returns the local virtual time for logical processor #localTimelineId.
     */
    long long const & getGlobalTime();

    /**
     * Set the virtual time snapshot \p vts to the local virtual time for
     * logical processor #localTimelineId.
     */
    void getCurrentTimeSnapshot(VirtualTimeSnapshot &vts);

  protected:
    /**
     * The virtual timeline this controller is responsible for.
     */
    MulticoreVirtualTimeline *multipleTimelines;

    /**
     * The logical processor ID this controller is responsible for.
     */
    int localTimelineId;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * TODO
 */
class StatisticsEnabledVirtualTimelineController : public VirtualTimelineController
{
  public:
    StatisticsEnabledVirtualTimelineController(VirtualTimelineController *_controller, TimeLoggingBehaviour *_timeLogging) : controller(_controller), timeLogging(_timeLogging) {}

    void commitCpuTimeProgress(VexThreadState *state);
    void commitIoTimeProgress(VexThreadState *state, const long long &actualIoProgress);
    void commitTimedWaitingProgress(VexThreadState *state);
    void commitNativeWaitingProgress(VexThreadState *state);
    void commitModelSimulationProgress(VexThreadState *state);
    void commitTimedOutIoProgress(VexThreadState *state);
    void commitBackgroundLoadExecutionTime(const long long &backgroundLoadExecutionTime);

    void updateResumingSuspendedThreadTimestamp(VexThreadState *state);
    void updateResumingSuspendedThreadResumedLastTimestamp(VexThreadState *state);
    void updateTimedOutWaitingThreadTimestamp(VexThreadState *state);
    void updateInterruptedWaitingThreadTimestamp(VexThreadState *state, const long long &interruptedTime);
    void updateBlockedThreadTimestamp(VexThreadState *state);

    long long getHowFarAheadInVirtualTimeTheThreadIs(VexThreadState *state);
    bool shouldBlockCoreProgress(const long long &schedulerTimeslot);
    bool disableCore();
    long long const &getGlobalTime();

    void getCurrentTimeSnapshot(VirtualTimeSnapshot &vts);

protected:
    VirtualTimelineController *controller;
    TimeLoggingBehaviour *timeLogging;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * TODO
 */
class VirtualTimeForwardLeapSnapshot
{
  public:
    VirtualTimeForwardLeapSnapshot(bool _allowed, long long _timeRemaining, long _timeout, long long _threadERT, int _underCreation, const struct vex_and_system_states &_vass);
    friend std::ostream & operator <<(std::ostream &outs, const VirtualTimeForwardLeapSnapshot &record);
    virtual ~VirtualTimeForwardLeapSnapshot();

  protected:
    bool allowed ;
    long long timeRemaining ;
    long timeout ;
    long long threadERT ;
    int underCreation ;
    struct vex_and_system_states *vass;
};

#endif /* TIMER_H_ */
