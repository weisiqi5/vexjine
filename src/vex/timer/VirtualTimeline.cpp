/*
 * VirtualTimeline.cpp
 *
 *  Created on: 28 Jun 2010
 *      Author: nb605
 */
#include <cassert>
#include <climits>
#include "ThreadState.h"
#include "Time.h"
#include "TimeLogger.h"
#include "VirtualTimeline.h"

VirtualTimeline::VirtualTimeline() {
  pthread_mutex_init(&mutex, NULL);
  reset();
}

VirtualTimeline::~VirtualTimeline() {
  pthread_mutex_destroy(&mutex);
}

void VirtualTimeline::reset() {
  globalVirtualTime = 0;
  unknownParallelTime = 0;
}

void VirtualTimeline::leapForwardTo(const long long &forwardTime) {
  pthread_mutex_lock(&mutex);
  if (forwardTime + unknownParallelTime > globalVirtualTime) {
    globalVirtualTime = forwardTime + unknownParallelTime;
  }
  unknownParallelTime = 0;
  pthread_mutex_unlock(&mutex);
}

void VirtualTimeline::addTimeOfUnknownRealTimeDurationAndSynchronize(const long long &forwardTime) {
  pthread_mutex_lock(&mutex);
  globalVirtualTime += forwardTime;
  pthread_mutex_unlock(&mutex);
}

void VirtualTimeline::addTimeOfThreadExecutingAtUnknownTime(const long long &forwardTime) {
  pthread_mutex_lock(&mutex);
  globalVirtualTime += forwardTime;
  unknownParallelTime += forwardTime;
  pthread_mutex_unlock(&mutex);
}

////////////////////////////////////////////////////////////////////////////////

MulticoreVirtualTimeline::MulticoreVirtualTimeline(const int &schedulers)
  : VirtualTimeline() {
    localCoreVirtualTimes = new long long[schedulers];
    for (int i = 0; i < schedulers; i++) {
      localCoreVirtualTimes[i] = 0; // LLONG_MAX
    }
    processors = schedulers;
  }

MulticoreVirtualTimeline::~MulticoreVirtualTimeline() {
  delete[] localCoreVirtualTimes;
}

void MulticoreVirtualTimeline::reset() {
  pthread_mutex_lock(&mutex);
  globalVirtualTime = 0;
  for (int i = 0; i < processors; i++) {
    localCoreVirtualTimes[i] = 0; // LLONG_MAX
  }
  pthread_mutex_unlock(&mutex);
}

void MulticoreVirtualTimeline::updateGlobalTimeTo(const long long &time) {
  if (globalVirtualTime < time) {
    globalVirtualTime = time;
  }
}

void MulticoreVirtualTimeline::leapForwardTo(const long long &time, const int &localTimelineId) {
  pthread_mutex_lock(&mutex);
  if (localCoreVirtualTimes[localTimelineId] < time) {
    localCoreVirtualTimes[localTimelineId] = time;
  }
  updateGlobalTimeTo(localCoreVirtualTimes[localTimelineId]);
  pthread_mutex_unlock(&mutex);
}

void MulticoreVirtualTimeline::addTimeOfUnknownRealTimeDurationAndSynchronize(const long long &forwardTime) {
  pthread_mutex_lock(&mutex);
  long long max = localCoreVirtualTimes[0];
  for (int i = 1; i < processors; i++) {
    if (localCoreVirtualTimes[i] > max) {
      max = localCoreVirtualTimes[i];
    }
  }
  max += forwardTime;
  for (int i = 0; i < processors; i++) {
    localCoreVirtualTimes[i] = max;
  }
  updateGlobalTimeTo(max);
  pthread_mutex_unlock(&mutex);
}

void MulticoreVirtualTimeline::addTimeOfThreadExecutingAtUnknownTime(const long long &forwardTime) {
  pthread_mutex_lock(&mutex);
  long long min = localCoreVirtualTimes[0];
  int minimumLocalTimelineId = 0;
  for (int i = 1; i < processors; i++) {
    if (min > localCoreVirtualTimes[i]) {
      min = localCoreVirtualTimes[i];
      minimumLocalTimelineId = i;
    }
  }
  localCoreVirtualTimes[minimumLocalTimelineId] += forwardTime;
  updateGlobalTimeTo(localCoreVirtualTimes[minimumLocalTimelineId]);
  pthread_mutex_unlock(&mutex);
}

bool MulticoreVirtualTimeline::shouldBlockCoreProgress(const long long &schedulerTimeslot, const int &localTimelineId) {
  pthread_mutex_lock(&mutex);
  bool ret = false;
  if (localCoreVirtualTimes[localTimelineId] - globalVirtualTime > schedulerTimeslot) {
    bool areAllCoresDisabled = disableLocalTimeline(localTimelineId);
    // if some cores are running then return true -> this core should not make pr
    ret = !areAllCoresDisabled;
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

bool MulticoreVirtualTimeline::disableLocalTimeline(const int &localTimelineId) {
  return false;
}

long long const & MulticoreVirtualTimeline::getLocalTimelineActivationTime(const long long &threadTime, const int &localTimelineId) {
  pthread_mutex_lock(&mutex);
  if (localCoreVirtualTimes[localTimelineId] < threadTime) {
    localCoreVirtualTimes[localTimelineId] = threadTime;
  }
  updateGlobalTimeTo(threadTime);
  pthread_mutex_unlock(&mutex);
  return localCoreVirtualTimes[localTimelineId];
}

long long const & MulticoreVirtualTimeline::getUpdatedGlobalTime() {
  return globalVirtualTime;
}

////////////////////////////////////////////////////////////////////////////////

DisablingMulticoreVirtualTimeline::DisablingMulticoreVirtualTimeline(const int &schedulers)
  : MulticoreVirtualTimeline(schedulers) {
    localCoreActive = new bool[schedulers];
    for (int i = 0; i < schedulers; i++) {
      localCoreActive[i] = false;
    }
    processors = schedulers;
  }

DisablingMulticoreVirtualTimeline::~DisablingMulticoreVirtualTimeline() {
  delete[] localCoreActive;
}

void DisablingMulticoreVirtualTimeline::reset() {
  pthread_mutex_lock(&mutex);
  globalVirtualTime = 0;
  for (int i = 0; i < processors; i++) {
    localCoreVirtualTimes[i] = 0; // LLONG_MAX
    localCoreActive[i] = false;
  }
  pthread_mutex_unlock(&mutex);
}

void DisablingMulticoreVirtualTimeline::updateGlobalTimeTo(const long long &time) {
  long long min = LLONG_MAX;
  for (int i = 0; i < processors; i++) {
    if (localCoreActive[i] && localCoreVirtualTimes[i] < min) {
      min = localCoreVirtualTimes[i];
    }
  }
  if (min != LLONG_MAX) {
    globalVirtualTime = min;
    // FIX: commented out, because when a local timeline was disabled at a localERT > GVT (global virtual time),
    // then the localERT was reverted to the GVT. This essentially replaced the progress that had already taken place,
    // which led the localERT to the higher value in the first place.
    // The idea of a disabled core is to wait in its localERT, until the GVT is within schedulerTimeslice distance from it.
    // for (int i = 0; i < processors; i++) {
    //     if (!localCoreActive[i]) {
    //         localCoreVirtualTimes[i] = min;
    //     }
    // }
  }
}

bool DisablingMulticoreVirtualTimeline::disableLocalTimeline(const int &localTimelineId) {
  pthread_mutex_lock(&mutex);
  localCoreActive[localTimelineId] = false;
  for (int i = 0; i < processors; i++) {
    if (localCoreActive[i]) {
      pthread_mutex_unlock(&mutex);
      return false;
    }
  }
  pthread_mutex_unlock(&mutex);
  return true;
}

long long const & DisablingMulticoreVirtualTimeline::getLocalTimelineActivationTime(const long long &threadTime, const int &localTimelineId) {
  pthread_mutex_lock(&mutex);
  localCoreActive[localTimelineId] = true;
  if (localCoreVirtualTimes[localTimelineId] < threadTime) {
    localCoreVirtualTimes[localTimelineId] = threadTime;
  }
  updateGlobalTimeTo(threadTime);
  pthread_mutex_unlock(&mutex);
  return localCoreVirtualTimes[localTimelineId];
}

long long const& DisablingMulticoreVirtualTimeline::getUpdatedGlobalTime() {
  pthread_mutex_lock(&mutex);
  long long min = LLONG_MAX;
  for (int i = 0; i < processors; i++) {
    if (localCoreActive[i] && localCoreVirtualTimes[i] < min) {
      min = localCoreVirtualTimes[i];
    }
  }
  if (min != LLONG_MAX) {
    globalVirtualTime = min;
  }
  pthread_mutex_unlock(&mutex);
  return globalVirtualTime;
}

//void DisablingMulticoreVirtualTimeline::leapForwardTo(const long long &time, const int &localTimelineId) {
//    lock();
//    if (localCoreVirtualTimes[localTimelineId] < time) {
//        localCoreVirtualTimes[localTimelineId] = time;
//    }
//    updateGlobalTime(localCoreVirtualTimes[localTimelineId]);
//    unlock();
//}
//void DisablingMulticoreVirtualTimeline::addTimeOfUnknownRealTimeDurationAndSynchronize(const long long &forwardTime, const int &id) {
//    lock();
//    localCoreVirtualTimes[localTimelineId] += forwardTime;
//    updateGlobalTime(localCoreVirtualTimes[localTimelineId]);
//    unlock();
//}

////////////////////////////////////////////////////////////////////////////////

SingleVirtualTimelineController::SingleVirtualTimelineController(VirtualTimeline *_timeline)
  : virtualTimeline(_timeline) {}

  void SingleVirtualTimelineController::commitCpuTimeProgress(VexThreadState *state) {
    Timers *threadTimer = state->getThreadTimers();
    virtualTimeline->leapForwardTo(threadTimer->getCurrentVirtualTimestampOfRunningThread());
  }

void SingleVirtualTimelineController::commitIoTimeProgress(VexThreadState *state, const long long &actualIoDuration) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->updateIoWaitingTimeFrom(virtualTimeline->getGlobalTime(), actualIoDuration);
  virtualTimeline->leapForwardTo(threadTimer->getEstimatedRealTime());
}

void SingleVirtualTimelineController::commitTimedWaitingProgress(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->updateWaitingTimeFrom(virtualTimeline->getGlobalTime());
  virtualTimeline->leapForwardTo(threadTimer->getEstimatedRealTime());
}

void SingleVirtualTimelineController::commitNativeWaitingProgress(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  virtualTimeline->addTimeOfThreadExecutingAtUnknownTime(threadTimer->getLocalTimeSinceLastResume());
}

void SingleVirtualTimelineController::commitBackgroundLoadExecutionTime(const long long &backgroundLoadExecutionTime) {
  virtualTimeline->addTimeOfUnknownRealTimeDurationAndSynchronize(backgroundLoadExecutionTime);
}

void SingleVirtualTimelineController::commitModelSimulationProgress(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  if (!state->isModelTimedWaiting()) {
    virtualTimeline->leapForwardTo(threadTimer->getCurrentVirtualTimestampOfRunningThread());
  }
}

void SingleVirtualTimelineController::commitTimedOutIoProgress(VexThreadState *state) {
  cout << "Invoking SingleVirtualTimelineController::commitTimedOutIoProgress" << endl;
  assert(false);
  Timers *threadTimer = state->getThreadTimers();
  virtualTimeline->leapForwardTo(threadTimer->getEstimatedRealTime());
}

void SingleVirtualTimelineController::updateResumingSuspendedThreadTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->leapForwardTo(virtualTimeline->getGlobalTime());
}

void SingleVirtualTimelineController::updateResumingSuspendedThreadResumedLastTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->updateTimesAfterResuming(virtualTimeline->getGlobalTime());
}

void SingleVirtualTimelineController::updateTimedOutWaitingThreadTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->leapForwardTo(virtualTimeline->getGlobalTime());
}

void SingleVirtualTimelineController::updateInterruptedWaitingThreadTimestamp(VexThreadState *state, const long long &interruptTime) {
  Timers *threadTimer = state->getThreadTimers();
  if (interruptTime == 0 && !threadTimer->shouldNotUpdateToGlobalTime()) {
    threadTimer->setEstimatedRealTime(virtualTimeline->getGlobalTime());
  } else {
    threadTimer->setEstimatedRealTime(interruptTime);
  }
}

void SingleVirtualTimelineController::updateBlockedThreadTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  long long currentGlobalTime = virtualTimeline->getGlobalTime();
  threadTimer->leapForwardTo(currentGlobalTime);
  threadTimer->setResumedLastAt(currentGlobalTime);
  threadTimer->getAndResetLocalTime();
}

long long SingleVirtualTimelineController::getHowFarAheadInVirtualTimeTheThreadIs(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  return threadTimer->getEstimatedRealTime() - virtualTimeline->getGlobalTime();
}

bool SingleVirtualTimelineController::shouldBlockCoreProgress(const long long &schedulerTimeslot) {
  return true;
}

bool SingleVirtualTimelineController::disableCore() {
  return false;
}

long long const &SingleVirtualTimelineController::getGlobalTime() {
  return virtualTimeline->getGlobalTime();
}

void SingleVirtualTimelineController::getCurrentTimeSnapshot(VirtualTimeSnapshot &vts) {
  vts = virtualTimeline->getGlobalTime();
}

////////////////////////////////////////////////////////////////////////////////

PassiveVirtualTimelineController::PassiveVirtualTimelineController(VirtualTimeline *_timeline)
  : SingleVirtualTimelineController(_timeline) {}

  void PassiveVirtualTimelineController::commitCpuTimeProgress(VexThreadState *state) {
    Timers *threadTimer = state->getThreadTimers();
    virtualTimeline->addTimeOfUnknownRealTimeDurationAndSynchronize(threadTimer->getAndResetLocalTime());
    threadTimer->setEstimatedRealTime(virtualTimeline->getGlobalTime());
  }

void PassiveVirtualTimelineController::commitIoTimeProgress(VexThreadState *state, const long long &actualIoDuration) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->updateIoWaitingTimeFrom(virtualTimeline->getGlobalTime(), actualIoDuration);
  virtualTimeline->addTimeOfUnknownRealTimeDurationAndSynchronize(threadTimer->getAndResetLocalTime());
  threadTimer->setEstimatedRealTime(virtualTimeline->getGlobalTime());
}

////////////////////////////////////////////////////////////////////////////////

/*
   void MultipleVirtualTimelinesController::commitCpuTimeProgress(ThreadState *state) {
   multipleTimelines->leapForwardTo(threadTimer->getCurrentVirtualTimestampOfRunningThread());
//tryForwardTimeLeap(state->getResumedLastAt()+state->getLocalTimeSinceLastResume());
//    localTimeLines->tryForwardTimeLeap(state->getResumedLastAt()+state->getLocalTimeSinceLastResume(), managerId);
}
void MultipleVirtualTimelinesController::commitIoTimeProgress(ThreadState *state) {
//    localTimeLines->tryForwardTimeLeap(state->getEstimatedRealTime(), managerId);
}
void MultipleVirtualTimelinesController::commitTimedWaitingProgress(ThreadState *state) {
//    localTimeLines->tryForwardTimeLeap(state->getEstimatedRealTime(), managerId);

}
void MultipleVirtualTimelinesController::commitNativeWaitingProgress(ThreadState *state) {

}
*/

MultipleVirtualTimelinesController::MultipleVirtualTimelinesController(MulticoreVirtualTimeline *_timelines, const int &_managerId)
  : multipleTimelines(_timelines), localTimelineId(_managerId) {}

  void MultipleVirtualTimelinesController::commitCpuTimeProgress(VexThreadState *state) {
    Timers *threadTimer = state->getThreadTimers();
    multipleTimelines->leapForwardTo(threadTimer->getCurrentVirtualTimestampOfRunningThread(), localTimelineId);
  }

void MultipleVirtualTimelinesController::commitIoTimeProgress(VexThreadState *state, const long long &actualIoDuration) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->updateIoWaitingTimeFrom(multipleTimelines->getLocalTime(localTimelineId), actualIoDuration);
  multipleTimelines->leapForwardTo(threadTimer->getEstimatedRealTime(), localTimelineId);
}

void MultipleVirtualTimelinesController::commitTimedWaitingProgress(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->updateWaitingTimeFrom(multipleTimelines->getLocalTime(localTimelineId));
  multipleTimelines->leapForwardTo(threadTimer->getEstimatedRealTime(), localTimelineId);
}

void MultipleVirtualTimelinesController::commitNativeWaitingProgress(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  multipleTimelines->addTimeOfThreadExecutingAtUnknownTime(threadTimer->getLocalTimeSinceLastResume());
}

void MultipleVirtualTimelinesController::commitBackgroundLoadExecutionTime(const long long &backgroundLoadExecutionTime) {
  multipleTimelines->addTimeOfThreadExecutingAtUnknownTime(backgroundLoadExecutionTime);
}

void MultipleVirtualTimelinesController::commitModelSimulationProgress(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  if (!state->isModelTimedWaiting()) {
    multipleTimelines->leapForwardTo(threadTimer->getEstimatedRealTime(), localTimelineId);
    threadTimer->leapForwardTo(multipleTimelines->getLocalTime(localTimelineId));
  }
}

void MultipleVirtualTimelinesController::commitTimedOutIoProgress(VexThreadState *state) {
  assert(false);
  Timers *threadTimer = state->getThreadTimers();
  multipleTimelines->leapForwardTo(threadTimer->getEstimatedRealTime(), localTimelineId);
}

void MultipleVirtualTimelinesController::updateResumingSuspendedThreadTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  long long currentLocalTimelineTime = multipleTimelines->getLocalTime(localTimelineId);
  threadTimer->leapForwardTo(currentLocalTimelineTime);
  if (currentLocalTimelineTime != threadTimer->getEstimatedRealTime()) {
    multipleTimelines->leapForwardTo(threadTimer->getEstimatedRealTime(), localTimelineId);
  }
}

void MultipleVirtualTimelinesController::updateResumingSuspendedThreadResumedLastTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  long long activationTime = multipleTimelines->getLocalTimelineActivationTime(threadTimer->getEstimatedRealTime(), localTimelineId);
  threadTimer->leapForwardTo(activationTime);
  threadTimer->updateTimesAfterResuming(activationTime);
  // BUG: small glitches due to other (Native Waiting) threads committing at the same time
  // assert(multipleTimelines->getLocalTime(localTimelineId) == threadTimer->getEstimatedRealTime());
}

void MultipleVirtualTimelinesController::updateInterruptedWaitingThreadTimestamp(VexThreadState *state, const long long &interruptTime) {
  Timers *threadTimer = state->getThreadTimers();
  if (interruptTime == 0 && !threadTimer->shouldNotUpdateToGlobalTime()) {
    threadTimer->setEstimatedRealTime(multipleTimelines->getLocalTime(localTimelineId));
  } else {
    threadTimer->setEstimatedRealTime(interruptTime);
  }
  // BUG: small glitches due to other (Native Waiting) threads committing at the same time
  // assert(multipleTimelines->getLocalTime(localTimelineId) == threadTimer->getEstimatedRealTime());
}

void MultipleVirtualTimelinesController::updateTimedOutWaitingThreadTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  threadTimer->leapForwardTo(multipleTimelines->getLocalTime(localTimelineId));
}

void MultipleVirtualTimelinesController::updateBlockedThreadTimestamp(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  long long currentGlobalTime = multipleTimelines->getUpdatedGlobalTime();
  threadTimer->leapForwardTo(currentGlobalTime);
  threadTimer->setResumedLastAt(currentGlobalTime);
  threadTimer->getAndResetLocalTime();
}

long long MultipleVirtualTimelinesController::getHowFarAheadInVirtualTimeTheThreadIs(VexThreadState *state) {
  Timers *threadTimer = state->getThreadTimers();
  return threadTimer->getEstimatedRealTime() - multipleTimelines->getGlobalTime();
}

bool MultipleVirtualTimelinesController::shouldBlockCoreProgress(const long long &schedulerTimeslot) {
  return multipleTimelines->shouldBlockCoreProgress(schedulerTimeslot, localTimelineId);
}

bool MultipleVirtualTimelinesController::disableCore() {
  return multipleTimelines->disableLocalTimeline(localTimelineId);
}

long long const &MultipleVirtualTimelinesController::getGlobalTime() {
  return multipleTimelines->getLocalTime(localTimelineId);
}

void MultipleVirtualTimelinesController::getCurrentTimeSnapshot(VirtualTimeSnapshot &vts) {
  vts.set(multipleTimelines->getGlobalTime(), multipleTimelines->getLocalTime(localTimelineId), localTimelineId);
}

////////////////////////////////////////////////////////////////////////////////

void StatisticsEnabledVirtualTimelineController::commitCpuTimeProgress(VexThreadState *state) {
  VirtualTimeSnapshot before; controller->getCurrentTimeSnapshot(before);
  controller->commitCpuTimeProgress(state);
  VirtualTimeSnapshot after; controller->getCurrentTimeSnapshot(after);
  timeLogging->logCpuTime(state, &before, &after);
}

void StatisticsEnabledVirtualTimelineController::commitIoTimeProgress(VexThreadState *state, const long long &actualIoProgress) {
  VirtualTimeSnapshot before; controller->getCurrentTimeSnapshot(before);
  controller->commitIoTimeProgress(state, actualIoProgress);
  VirtualTimeSnapshot after; controller->getCurrentTimeSnapshot(after);
  timeLogging->logIoTime(state, &before, &after);
}

void StatisticsEnabledVirtualTimelineController::commitTimedWaitingProgress(VexThreadState *state) {
  VirtualTimeSnapshot before; controller->getCurrentTimeSnapshot(before);
  controller->commitTimedWaitingProgress(state);
  VirtualTimeSnapshot after; controller->getCurrentTimeSnapshot(after);
  timeLogging->logTimedWaitingTime(state, &before, &after);
}

void StatisticsEnabledVirtualTimelineController::commitNativeWaitingProgress(VexThreadState *state) {
  VirtualTimeSnapshot before; controller->getCurrentTimeSnapshot(before);
  controller->commitNativeWaitingProgress(state);
  VirtualTimeSnapshot after; controller->getCurrentTimeSnapshot(after);
  timeLogging->logNativeWaitingTime(state, &before, &after);
}

void StatisticsEnabledVirtualTimelineController::commitBackgroundLoadExecutionTime(const long long &backgroundLoadExecutionTime) {
  VirtualTimeSnapshot before; controller->getCurrentTimeSnapshot(before);
  controller->commitBackgroundLoadExecutionTime(backgroundLoadExecutionTime);
  VirtualTimeSnapshot after; controller->getCurrentTimeSnapshot(after);
  timeLogging->logBackgroundLoadExecutionTime(&before, &after);
}

void StatisticsEnabledVirtualTimelineController::commitModelSimulationProgress(VexThreadState *state) {
  VirtualTimeSnapshot before; controller->getCurrentTimeSnapshot(before);
  controller->commitModelSimulationProgress(state);
  VirtualTimeSnapshot after; controller->getCurrentTimeSnapshot(after);
  timeLogging->logModelSimulationTime(state, &before, &after);
}

void StatisticsEnabledVirtualTimelineController::commitTimedOutIoProgress(VexThreadState *state) {
  VirtualTimeSnapshot before; controller->getCurrentTimeSnapshot(before);
  controller->commitTimedOutIoProgress(state);
  VirtualTimeSnapshot after; controller->getCurrentTimeSnapshot(after);
  timeLogging->logTimedOutIoTime(state, &before, &after);
}

void StatisticsEnabledVirtualTimelineController::updateResumingSuspendedThreadTimestamp(VexThreadState *state) {
  controller->updateResumingSuspendedThreadTimestamp(state);
}

void StatisticsEnabledVirtualTimelineController::updateResumingSuspendedThreadResumedLastTimestamp(VexThreadState *state) {
  controller->updateResumingSuspendedThreadResumedLastTimestamp(state);
}

void StatisticsEnabledVirtualTimelineController::updateInterruptedWaitingThreadTimestamp(VexThreadState *state, const long long &interruptTime) {
  controller->updateInterruptedWaitingThreadTimestamp(state, interruptTime);
}

void StatisticsEnabledVirtualTimelineController::updateTimedOutWaitingThreadTimestamp(VexThreadState *state) {
  controller->updateTimedOutWaitingThreadTimestamp(state);
}

void StatisticsEnabledVirtualTimelineController::updateBlockedThreadTimestamp(VexThreadState *state) {
  controller->updateBlockedThreadTimestamp(state);
}

long long StatisticsEnabledVirtualTimelineController::getHowFarAheadInVirtualTimeTheThreadIs(VexThreadState *state) {
  return controller->getHowFarAheadInVirtualTimeTheThreadIs(state);
}

bool StatisticsEnabledVirtualTimelineController::shouldBlockCoreProgress(const long long &schedulerTimeslot) {
  return controller->shouldBlockCoreProgress(schedulerTimeslot);
}

bool StatisticsEnabledVirtualTimelineController::disableCore() {
  return controller->disableCore();
}

long long const &StatisticsEnabledVirtualTimelineController::getGlobalTime() {
  return controller->getGlobalTime();
}

void StatisticsEnabledVirtualTimelineController::getCurrentTimeSnapshot(VirtualTimeSnapshot &vts) {
  controller->getCurrentTimeSnapshot(vts);
}

////////////////////////////////////////////////////////////////////////////////

VirtualTimeSnapshot& VirtualTimeSnapshot::operator-=(const VirtualTimeSnapshot &rhs) {
  if (localTimelineId == rhs.localTimelineId) {
    globalTime -= rhs.globalTime;
    localTime -= rhs.localTime;
  } else {
    cout << "Tried to compare time differences between different cores" << endl;
    assert(false);
  }
  return *this;
}

////////////////////////////////////////////////////////////////////////////////

VirtualTimeForwardLeapSnapshot::VirtualTimeForwardLeapSnapshot(bool _allowed, long long _timeRemaining, long _timeout, long long _threadERT, int _underCreation, const struct vex_and_system_states &_vass) {
  allowed = _allowed;
  timeRemaining = _timeRemaining;
  timeout = _timeout;
  threadERT = _threadERT;
  underCreation =  _underCreation;
  vass = new struct vex_and_system_states(_vass);
}

VirtualTimeForwardLeapSnapshot::~VirtualTimeForwardLeapSnapshot() {
  delete vass;
}

std::ostream &operator<<(std::ostream &outs, const VirtualTimeForwardLeapSnapshot &record) {
  outs << "VL" << "\t" << "rem" << "\t" << "T/o" << "\t" << "ERT" << "\t" <<
    "SPN"<< "\t" << "W" << "\t" <<
    "NW"  << "\t" << "IO" << "\t" <<
    "IO-bl" << "\t" << "SUSP" << "\t" << "RUN" << "\t" <<
    "Join" << "\t" << "Reg" << "\t" << "Awa" << "\t" <<
    "Mod" << "\t" <<
    "R" << " " << "S"<< " " << "D" << " " <<
    "T" << " " << "Z" << " " << endl;

  outs << record.allowed << "\t" << record.timeRemaining/1e6 << "\t" << record.timeout/1e6 << "\t" << record.threadERT/1e6 << "\t" <<
    record.underCreation << "\t" << record.vass->waitingThreads << "\t" <<
    record.vass->nwThreads << "\t" << record.vass->activeInIoThreads << "\t" <<
    record.vass->blockedInIoThreads << "\t" << record.vass->suspended << "\t" <<
    record.vass->running << "\t" <<
    record.vass->isAwakeningAfterJoin << "\t" << record.vass->isRegistering << "\t" << record.vass->isAwakenInVex << "\t" <<
    record.vass->isWaitingRealCodeToCompleteAfterModelSimulation << "\t" <<
    record.vass->systemRunning << " " <<
    record.vass->systemSleeping << " " << record.vass->systemD << " " <<
    record.vass->systemStopped << " " << record.vass->systemZombies << " " <<
    std::endl;

  return outs;
};
