/*
 * MethodCallInfo.h
 *
 *  Created on: 17 Feb 2010
 *      Author: nb605
 */

#ifndef METHODCALLINFO_H_
#define METHODCALLINFO_H_

#include "PerformanceMeasure.h"
#include "MethodData.h"

/**
 * XXX Stores timestamps and other information on a profiled method executed
 * by a VEX monitored thread.
 */
class MethodCallInfo
{
  public:
    /**
     * Construct a new MethodCallInfo with default, nil values.
     */
    MethodCallInfo();

    /**
     * Construct a new MethodCallInfo with data provided by the arguments.
     */
    MethodCallInfo(const int &_methodId, const long long &virtualTime, const long long &estimatedRealTime, const long long &monitorWaitingTime, const long long &ioWaitingTime);

    /**
     * Destructor.
     */
    virtual ~MethodCallInfo();

    /**
     * Return #recursive.
     */
    bool isRecursive() {
        return recursive;
    }

    /**
     * Set #recursive to true.
     */
    void setRecursive() {
        recursive = true;
    }

    /**
     * Change the information stored in this MethodCallInfo with new data
     * provided by the arguments.
     */
    void setInfo(const int &_methodId, const long long &virtualTime, const long long &estimatedRealTime, const long long &monitorWaitingTime, const long long &ioWaitingTime);

    /**
     * Return #methodId.
     */
    int getMethodId() {
        return methodId;
    }

    /**
     * Return #entry_timestamp, taking into account of #lost_time.
     */
    long long getTime() {
        return entry_timestamp - lost_time;
    }

    /**
     * Return #real_entry_timestamp, taking into account of #lost_time.
     */
    long long getRealTime() {
        return real_entry_timestamp - lost_time;
    }

    /**
     * Return #lost_time, taking into account of #lost_time_callee.
     */
    long long getTotalLostTime() {
        return lost_time + lost_time_callee;
    }

    /**
     * Return #shouldResetVTFactor.
     */
    bool getShouldResetVTFactor() {
        return shouldResetVTFactor;
    }

    /**
     * Set #shouldResetVTFactor to \p value.
     */
    void setShouldResetVTFactor(bool value) {
        shouldResetVTFactor = value;
    }

    /**
     * TODO
     */
    void updateCalleeTime(const long long &diff, const long long &real_diff, const long long &callee_lost_time, const long long &callee_monitor_wait, const long long &callee_io_wait) {
        callee_time += diff;
        real_callee_time += real_diff;
        callee_monitor_waiting_timestamp += callee_monitor_wait;
        callee_io_waiting_timestamp += callee_io_wait;
    }

    /**
     * Return the difference between the #entry_timestamp of this MethodCallInfo
     * and the #entry_timestamp of \p start.
     */
    long long getInclusiveCpuTimeDifferenceFrom(MethodCallInfo *start) {
        return entry_timestamp - start->entry_timestamp;
    }

    /**
     * Return the difference between the #real_entry_timestamp of this
     * MethodCallInfo and the #real_entry_timestamp of \p start.
     */
    long long getInclusiveRealTimeDifferenceFrom(MethodCallInfo *start) {
        return real_entry_timestamp - start->real_entry_timestamp;
    }

    /**
     * Return the difference between the #monitor_waiting_timestamp of this
     * MethodCallInfo and the #monitor_waiting_timestamp of \p start.
     */
    long long getInclusiveMonitorWaitDifferenceFrom(MethodCallInfo *start) {
        return monitor_waiting_timestamp - start->monitor_waiting_timestamp;
    }

    /**
     * Return the difference between the #io_waiting_timestamp of this
     * MethodCallInfo and the #io_waiting_timestamp of \p start.
     */
    long long getInclusiveIoWaitDifferenceFrom(MethodCallInfo *start) {
        return io_waiting_timestamp - start->io_waiting_timestamp;
    }

    /**
     * Subtract #entry_timestamp and #real_entry_timestamp from MethodCallInfo
     * \p callinfo from this MethodCallInfo, taking into account of any lost
     * time.
     */
    MethodCallInfo& operator-= (const MethodCallInfo& callinfo);

    /**
     * TODO
     */
    void logTimesOnMethodExit(PerformanceMeasure *measure, MethodCallInfo *entryCallinfo, MethodCallInfo *callingMethodInfo);

    // void decreaseThreadsExecutingMethodCounter();
    // void setGlobalMethodDataOnEntry(MethodData *data);

  protected:
    /**
     * The method ID this MethodCallInfo holds information for.
     */
    int methodId;

    /**
     * Virtual timestamp at method entry.
     */
    long long entry_timestamp;

    /**
     * Estimated real timestamp at method entry.
     */
    long long real_entry_timestamp;

    /**
     * CPU time spent in callee methods.
     */
    long long callee_time;

    /**
     * Estimated real time spent in callee methods.
     */
    long long real_callee_time;

    /**
     * Maximum predicted virtual time from callee methods.
     *
     * Since v2.8, virtual time from callee methods smaller than this should not
     * be taken into account for this method.
     */
    long long maximumPredictedVT;

    /**
     * Maximum predicted estimated real time from callee methods.
     *
     * Since v2.8, estimated real time from callee methods smaller than this
     * should not be taken into account for this method.
     */
    long long maximumEstimatedRealTime;

    /**
     * Time spent waiting for acquiring a monitor.
     */
    long long monitor_waiting_timestamp;

    /**
     * Time spent waiting for I/O completion.
     */
    long long io_waiting_timestamp;

    /**
     * TODO
     */
    long long callee_monitor_waiting_timestamp;

    /**
     * TODO
     */
    long long callee_io_waiting_timestamp;

    /**
     * TODO
     */
    long long lost_time;

    /**
     * TODO
     */
    long long lost_time_callee;

    /**
     * Whether or not the thread which called the method corresponding to
     * #methodId should reset its local virtual time scaling factor upon exit.
     */
    bool shouldResetVTFactor;

    /**
     * Whether or not this method is recursive.
     */
    bool recursive;

    /**
     * Unused.
     */
    MethodData *methodData;

    /**
     * Unused.
     *
     * 4 bytes pointing to the length of the string in the
     * current_method_trace_bythread that corresponds to this event's method
     * starting occurrance (used for pop).
     */
    int method_trace_ptr;

    /**
     * Unused.
     */
    MethodCallInfo *callingMethod;
};

/**
 * Same as a MethodCallInfo, but also contains a PerformanceMeasure.
 */
class StackTraceInfo : public MethodCallInfo
{
  public:
    /**
     * Constructs a new StackTraceInfo with default, nil values.
     */
    StackTraceInfo() : MethodCallInfo() {
        correspondingStackTraceMeasure = 0;
    }

    /**
     * Construct a new StackTraceInfo with data provided by the arguments.
     */
    StackTraceInfo(const int &_methodId, const long long &virtualTime, const long long &estimatedRealTime, const long long &monitorWaitingTime, const long long &ioWaitingTime, PerformanceMeasure *_correspondingStackTraceMeasure) :
        MethodCallInfo(_methodId, virtualTime, estimatedRealTime, monitorWaitingTime, ioWaitingTime) {
        correspondingStackTraceMeasure = _correspondingStackTraceMeasure;
    }

    /**
     * Destructor.
     */
    ~StackTraceInfo();

    /**
     * Return #correspondingStackTraceMeasure.
     */
    PerformanceMeasure *getCorrespondingStackTraceMeasure() {
        return correspondingStackTraceMeasure;
    }

    /**
     * Change the information stored in this StackTraceInfo with new data
     * provided by the arguments.
     */
    void setInfo(const int &_methodId, const long long &virtualTime, const long long &estimatedRealTime, const long long &monitorWaitingTime, const long long &ioWaitingTime, PerformanceMeasure *_correspondingStackTraceMeasure);

  private:
    /**
     * Performance measure associated with this stack trace.
     */
    PerformanceMeasure *correspondingStackTraceMeasure;
};

/**
 * Same as a MethodCallInfo, but also contains a long double that denotes the
 * path to the trace.
 */
class PrimeNumberStackTraceInfo : public MethodCallInfo
{
  public:
    /**
     * Constructs a new PrimeNumberStackTraceInfo with default, nil values.
     */
    PrimeNumberStackTraceInfo() : MethodCallInfo() {
        traceId = 0;
    }

    /**
     * Construct a new PrimeNumberStackTraceInfo with data provided by the
     * arguments.
     */
    PrimeNumberStackTraceInfo(const int &_methodId, const long long &virtualTime, const long long &estimatedRealTime, const long long &monitorWaitingTime, const long long &ioWaitingTime, const long double &_traceId) :
        MethodCallInfo(_methodId, virtualTime, estimatedRealTime, monitorWaitingTime, ioWaitingTime) {
        traceId = _traceId;
    }

    /**
     * Destructor.
     */
    ~PrimeNumberStackTraceInfo();

    /**
     * Change the information stored in this PrimeNumberStackTraceInfo with new
     * data provided by the arguments.
     */
    void setInfo(const int &_methodId, const long long &virtualTime, const long long &estimatedRealTime, const long long &monitorWaitingTime, const long long &ioWaitingTime, const long double &_traceId);

  private:
    long double traceId;
};

#endif /* METHODCALLINFO_H_ */
