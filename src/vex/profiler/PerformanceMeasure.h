/*
 * PerformanceMeasure.h: Class that receives samples of performance data and calculates the mean of the samples
 *
 *  Created on: Jul 30, 2008
 *      Author: richard
 */

#ifndef PERFORMANCEMEASURE_H_
#define PERFORMANCEMEASURE_H_

#define MAXIMUM_SAMPLES_PER_MEASURE 1
// Hard-coded method id of the method used for profiling of instrumentation delays
#define INSTRUMENTATION_OVERHEAD_PROFILING_METHOD_ID 50

#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <typeinfo>

/***
 * The SampleStorageBehaviour is an interface that defines how the samples for each performance measure (which can be on a method or stack-trace
 * level) need to be stored. It might be indifferent to store each sample (NoSampleStorage), or we might want a limited number of samples
 * (LimitedSamplesStorage) or an unlimited one (UnlimitedSamplesStorage), though the latter might bear a heavy memory overhead
 */

/**
 * XXX Interface that defines how the samples for each performance measure (can be
 * on a method or stack trace level) are stored.
 */
class SampleStorageBehaviour
{
  public:
    /**
     * Constructor.
     */
    SampleStorageBehaviour() {
        samples = 0;
    }

    /**
     * Should be implemented to initialise the underlying data structure storing
     * samples.
     */
    virtual void init() = 0;

    /**
     * Add a performance sample \p sample.
     */
    virtual void addSample(const long long &sample) = 0;

    /**
     * Return #samples.
     */
    unsigned int const& getSamples() const {
        return samples;
    }

    /**
     * Add #samples from \p ssb.
     */
    virtual SampleStorageBehaviour& operator+=(SampleStorageBehaviour& ssb) {
        samples += ssb.samples;
        return *this;
    }

  protected:
    friend std::ostream& operator<<(std::ostream &outs, const SampleStorageBehaviour &record) {
        return record.printSamples(outs);
    };

    /**
     * Nothing is done; \p outs is returned as-is.
     */
    virtual std::ostream &printSamples(std::ostream &outs) const {
        return outs;
    }

    /**
     * Count of stored samples.
     */
    unsigned int samples;
};

/**
 * An implementation of SampleStorageBehaviour, NoSampleStorage does not store
 * any performance samples and only keeps a count of the number of samples.
 */
class NoSampleStorage : public SampleStorageBehaviour {
  public:
    /**
     * Constructor.
     */
    NoSampleStorage() : SampleStorageBehaviour() {};

    /**
     * Do nothing.
     */
    void init() {};

    /**
     * Do not store \p sample, instead just increment the count of samples.
     */
    void addSample(const long long &sample) {
        ++samples;
    }
};

/**
 * XXX An implementation of SampleStorageBehaviour, LimitedSamplesStorage stores
 * a limited number of samples (up to MAXIMUM_SAMPLES_PER_MEASURE) inside an
 * array.
 */
class LimitedSamplesStorage : public SampleStorageBehaviour {
  public:
    /**
     * Constructor.
     */
    LimitedSamplesStorage() : SampleStorageBehaviour() {};

    /**
     * Initialises the array #storedSamples.
     *
     * FIXME This SHOULD be called inside the constructor, but is currently
     * by PerformanceMeasure!
     */
    void init() {
        for(int i = 0; i < MAXIMUM_SAMPLES_PER_MEASURE; i++) {
            storedSamples[i] = 0;
        }
    }

    /**
     * Add a sample if MAXIMUM_SAMPLES_PER_MEASURE for this performance measure
     * has not been exceeded yet.
     */
    void addSample(const long long &sample) {
        if (samples < MAXIMUM_SAMPLES_PER_MEASURE) {
            storedSamples[samples] = sample;
        }
        ++samples;
    }

    /**
     * Add samples from \p ssb.
     */
    SampleStorageBehaviour& operator+=(SampleStorageBehaviour &ssb) {
        LimitedSamplesStorage &ssb_temp = dynamic_cast<LimitedSamplesStorage &>(ssb);
        int count = samples;
        int count2 = 0;
        while (count < MAXIMUM_SAMPLES_PER_MEASURE) {
            storedSamples[count] = ssb_temp.storedSamples[count2];
        }
        samples += ssb.getSamples();
        return *this;
    }

  protected:
    friend std::ostream & operator<<(std::ostream &outs, const LimitedSamplesStorage &record);

    /**
     * Send the array of stored samples to \p outs, each delimited by ','.
     */
    virtual std::ostream &printSamples(std::ostream &outs) const {
        outs << ",,Stored samples:";
        for (unsigned int i = 0; i < samples; i++) {
            outs << "," << storedSamples[i];
        }
        return outs;
    }

    /**
     * Array of stored samples.
     */
    long long storedSamples[MAXIMUM_SAMPLES_PER_MEASURE];
};

/**
 * XXX Receives samples of performance data and calculates the mean.
 */
class PerformanceMeasure
{
    friend std::ostream & operator << (std::ostream &outs, PerformanceMeasure &record) {
        return record.printMeansInMSec(outs);//monitorWaitExcludingCalleeMethods << "," << record.ioWaitExcludingCalleeMethods  << "," << record.monitorWaitIncludingCalleeMethods  << "," << record.ioWaitIncludingCalleeMethods << "," << record.cpuTimeExcludingCalleeMethods << "," << record.estimatedRealTimeExcludingCalleeMethods << "," << record.cpuTimeIncludingCalleeMethods << "," << record.estimatedRealTimeIncludingCalleeMethods << ",";
    };

  public:
    /**
     * Construct a performance measure for a profiled method \p methodId.
     */
    PerformanceMeasure(const int &methodId);

    /**
     * Construct a performance measure for a profiled method \p methodId with
     * the performance measure of the previous calling (caller) method.
     */
    PerformanceMeasure(const int &methodId, PerformanceMeasure *_callingMethod);

    /**
     * Destructor.
     */
    virtual ~PerformanceMeasure();

    /**
     * Add a sample which consists of a virtual time and an estimated real time.
     *
     * The summed values refer to caller and callee methods.
     */
    void add(const long long &virtual_value, const long long &real_value, const long long &sample_cpuTimeIncludingCalleeMethods, const long long &sample_estimatedRealTimeIncludingCalleeMethods, const long long &monWait, const long long &ioWait, const long long &incl_monWait, const long long &incl_ioWait);

    /**
     * Add additional information on callee time, callee lost time and lost
     * time.
     */
    void moreInfo(long long _callee_time, long long _callee_lost, long long _lost_time);

    /**
     * Add another PerformanceMeasure \p callinfo to this PerformanceMeasure.
     */
    PerformanceMeasure& operator+=(PerformanceMeasure &callinfo);

    /**
     * Compute the mean of \p metric by dividing \p metric with the number of
     * samples.
     */
    double getMean(const long long &metric) const;

    /**
     * Return the number of current samples.
     */
    unsigned int const &getMethodInvocations() const;

    /**
     * Return #cpuTimeExcludingCalleeMethods.
     */
    long long getCpuTimeExcludingCalleeMethods();

    /**
     * Return #cpuTimeIncludingCalleeMethods.
     */
    long long getCpuTimeIncludingCalleeMethods();

    /**
     * Return #estimatedRealTimeIncludingCalleeMethods.
     */
    long long getEstimatedRealTimeIncludingCalleeMethods();

    /**
     * Return #methodId.
     */
    int getMethodId();

    /**
     * Return #_methodId.
     */
    void setMethodId(const int &_methodId);

    /**
     * Return #methodName if it exists.
     */
    const char* getMethodNameString() {
        if (methodName != NULL) {
            return methodName;
        } else {
            return "Unknown method from PM";
        }
    }

    /**
     * Return the mean estimated real time including callee methods.
     */
    double getMeanERT() {
        return getMean(estimatedRealTimeIncludingCalleeMethods);
    }

    /**
     * Prints the mean of all relevant metrics to \p outs.
     */
    std::ostream & printMeansInMSec(std::ostream &outs) {
        // This fix has to do with the fact that I/O is also added as localTime, which increases CPU time as well - we compensate for this here
        cpuTimeExcludingCalleeMethods -= ioWaitExcludingCalleeMethods; if (cpuTimeExcludingCalleeMethods < 0) cpuTimeExcludingCalleeMethods = 0;
        long long suspendedTimeExcludingCalleeMethods = estimatedRealTimeExcludingCalleeMethods - cpuTimeExcludingCalleeMethods - monitorWaitExcludingCalleeMethods - ioWaitExcludingCalleeMethods;

        // This fix has to do with the fact that I/O is also added as localTime, which increases CPU time as well - we compensate for this here
        cpuTimeIncludingCalleeMethods -= ioWaitIncludingCalleeMethods; if (cpuTimeIncludingCalleeMethods < 0) cpuTimeIncludingCalleeMethods = 0;
        long long suspendedTimeIncludingCalleeMethods = estimatedRealTimeIncludingCalleeMethods - cpuTimeIncludingCalleeMethods - monitorWaitIncludingCalleeMethods - ioWaitIncludingCalleeMethods;

        outs << std::fixed << std::setprecision(3) <<
            getMean(estimatedRealTimeExcludingCalleeMethods)/1e6 << "," <<
            getMean(cpuTimeExcludingCalleeMethods)/1e6 << "," <<
            getMean(monitorWaitExcludingCalleeMethods)/1e6 << "," <<
            getMean(ioWaitExcludingCalleeMethods)/1e6  << "," <<
            getMean(suspendedTimeExcludingCalleeMethods)/1e6  << "," <<

            getMean(estimatedRealTimeIncludingCalleeMethods)/1e6 << "," <<
            getMean(cpuTimeIncludingCalleeMethods)/1e6 << "," <<
            getMean(monitorWaitIncludingCalleeMethods)/1e6  << "," <<
            getMean(ioWaitIncludingCalleeMethods)/1e6 << "," <<
            getMean(suspendedTimeIncludingCalleeMethods)/1e6 << *ertSamplesStorage;
        return outs;
    }

    /**
     * The method used for profiling instrumentation delays has a hard-coded
     * method ID; return true if the current method associated with this
     * PerformanceMeasure is this method.
     */
    bool isTheInstrumentationProfilingMethod() {
        return methodId == INSTRUMENTATION_OVERHEAD_PROFILING_METHOD_ID;
    }

    /**
     * Add \p submethodTraceMeasure to #subMethods, creating one if it does not
     * currently exist.
     */
    void addSubmethodTrace(PerformanceMeasure *submethodTraceMeasure) {
        if (subMethods == NULL) {
            subMethods = new std::vector<PerformanceMeasure *>();
        }
        subMethods->push_back(submethodTraceMeasure);
        submethodTraceMeasure->setCallingMethod(this);
    }

    /**
     * Return if the method associated with this PerformanceMeasure is a root
     * method (does not have a parent caller).
     */
    bool isRoot() {
        return callingMethod == NULL;
    }

    /**
     * Set #callingMethod to \p _callingMethod.
     */
    void setCallingMethod(PerformanceMeasure *_callingMethod) {
        callingMethod = _callingMethod;
    }

    /**
     * Return #callingMethod.
     */
    PerformanceMeasure* getCallingMethod() {
        return callingMethod;
    }

    /**
     * Return #subMethods, creating one if it does not currently exist.
     */
    std::vector<PerformanceMeasure*>* initGetSubMethods() {
        if (subMethods == NULL) {
            subMethods = new std::vector<PerformanceMeasure *>;
        }
        return subMethods;
    }

    /**
     * Return #subMethods.
     */
    std::vector<PerformanceMeasure*>* getSubMethods() {
        return subMethods;
    }

    /**
     * Set #subMethods to \p _subMethods.
     */
    void setSubMethods(std::vector<PerformanceMeasure*> *_subMethods) {
        subMethods = _subMethods;
    }

    /**
     * Set #recursive to \p value.
     */
    void setRecursive(bool value) {
        recursive = value;
        if (!value) {
            alreadyAdapted = true;
        }
    }

    /**
     * Return #recursive.
     */
    bool isRecursive() {
        return recursive && !alreadyAdapted;
    }

    /**
     * Return #methodName.
     */
    const char *getMethodName() {
        return methodName;
    }

    /**
     * Set #methodName to \p _methodName.
     */
    void setMethodName(const char *_methodName) {
        methodName = _methodName;
    }

  protected:
    /**
     * Initialise this object with default, nil values.
     */
    void init(const int &_methodId);

    /**
     * Name of the method this object is associated with.
     */
    const char *methodName;

    /**
     * Method ID of the method this object is associated with.
     */
    int methodId;

    /**
     * PerformanceMeasure object of this method's parent caller.
     */
    PerformanceMeasure *callingMethod;

    /**
     * Vector of PerformanceMeasure objects for callee methods that the method
     * associated with this PerformanceMeasure calls.
     */
    std::vector<PerformanceMeasure *> *subMethods;

    /**
     * Whether the method this object is recursive.
     */
    bool recursive;
    bool alreadyAdapted;

    /**
     * Stores the recorded performance samples.
     */
    SampleStorageBehaviour *ertSamplesStorage;

    long long cpuTimeExcludingCalleeMethods;
    long long estimatedRealTimeExcludingCalleeMethods;

    long long cpuTimeIncludingCalleeMethods;
    long long estimatedRealTimeIncludingCalleeMethods;

    long long monitorWaitExcludingCalleeMethods;
    long long ioWaitExcludingCalleeMethods;

    long long monitorWaitIncludingCalleeMethods;
    long long ioWaitIncludingCalleeMethods;

    long long callee_time;
    long long callee_lost_time;
    long long lost_time;
};

#endif /* PERFORMANCEMEASURE_H_ */
