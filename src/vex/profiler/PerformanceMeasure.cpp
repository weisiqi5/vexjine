/*
 * PerformanceMeasure.cpp
 *
 *  Created on: Jul 30, 2008
 *      Author: richard
 */

#include "PerformanceMeasure.h"

#include <iostream>

PerformanceMeasure::PerformanceMeasure(const int &_methodId) {
    ertSamplesStorage = new NoSampleStorage();
    init(_methodId);
}

PerformanceMeasure::PerformanceMeasure(const int &_methodId, PerformanceMeasure *_callingMethod) {
    ertSamplesStorage = new NoSampleStorage();
    init(_methodId);
    callingMethod = _callingMethod;
}

PerformanceMeasure::~PerformanceMeasure() {
    delete ertSamplesStorage;
}

void PerformanceMeasure::add(const long long &virtual_value, const long long &real_value, const long long &sample_cpuTimeIncludingCalleeMethods, const long long &sample_estimatedRealTimeIncludingCalleeMethods,
                             const long long &monWait, const long long &ioWait, const long long &incl_monWait, const long long &incl_ioWait) {
    cpuTimeExcludingCalleeMethods += virtual_value;
    estimatedRealTimeExcludingCalleeMethods += real_value;

    cpuTimeIncludingCalleeMethods += sample_cpuTimeIncludingCalleeMethods;
    estimatedRealTimeIncludingCalleeMethods += sample_estimatedRealTimeIncludingCalleeMethods;

    monitorWaitExcludingCalleeMethods += monWait;
    ioWaitExcludingCalleeMethods += ioWait;

    monitorWaitIncludingCalleeMethods += incl_monWait;
    ioWaitIncludingCalleeMethods += incl_ioWait;

    ertSamplesStorage->addSample(sample_estimatedRealTimeIncludingCalleeMethods);
}

void PerformanceMeasure::moreInfo(long long _callee_time, long long _callee_lost, long long _lost_time) {
    lost_time += _lost_time;
    callee_lost_time += _callee_lost;
    callee_time += _callee_time;
}

PerformanceMeasure& PerformanceMeasure::operator+=(PerformanceMeasure& pm) {
    cpuTimeExcludingCalleeMethods += pm.cpuTimeExcludingCalleeMethods;
    estimatedRealTimeExcludingCalleeMethods += pm.estimatedRealTimeExcludingCalleeMethods;

    cpuTimeIncludingCalleeMethods += pm.cpuTimeIncludingCalleeMethods;
    estimatedRealTimeIncludingCalleeMethods += pm.estimatedRealTimeIncludingCalleeMethods;

    *ertSamplesStorage += *pm.ertSamplesStorage;

    if (methodName == NULL && pm.methodName != NULL) {
        methodName = pm.methodName;
    }
    return *this;
}

double PerformanceMeasure::getMean(const long long &metric) const {
    if (metric == 0) return 0.0;
    return (double)metric/(double)ertSamplesStorage->getSamples();
}

unsigned int const &PerformanceMeasure::getMethodInvocations() const {
    return ertSamplesStorage->getSamples();
}

long long PerformanceMeasure::getCpuTimeExcludingCalleeMethods() {
    return cpuTimeExcludingCalleeMethods;
}

long long PerformanceMeasure::getCpuTimeIncludingCalleeMethods() {
    return cpuTimeIncludingCalleeMethods;
}

long long PerformanceMeasure::getEstimatedRealTimeIncludingCalleeMethods() {
    return estimatedRealTimeIncludingCalleeMethods;
}

int PerformanceMeasure::getMethodId() {
    return methodId;
}

void PerformanceMeasure::setMethodId(const int &_methodId) {
    methodId = _methodId;
}

void PerformanceMeasure::init(const int &_methodId) {
    cpuTimeExcludingCalleeMethods = 0;
    estimatedRealTimeExcludingCalleeMethods = 0;

    cpuTimeIncludingCalleeMethods = 0;
    estimatedRealTimeIncludingCalleeMethods = 0;

    monitorWaitExcludingCalleeMethods = 0;
    ioWaitExcludingCalleeMethods = 0;

    monitorWaitIncludingCalleeMethods = 0;
    ioWaitIncludingCalleeMethods = 0;
    ertSamplesStorage->init();
    lost_time = 0;
    callee_lost_time = 0;
    callee_time = 0;

    methodId = _methodId;

    subMethods = NULL;
    callingMethod = NULL;
    methodName = NULL;

    recursive = false;
    alreadyAdapted = false;
}
