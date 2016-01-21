/*
 * Scheduling.h
 *
 *  Created on: 21 Apr 2011
 *      Author: root
 */

#ifndef SCHEDULING_H_
#define SCHEDULING_H_

#include <pthread.h>
#include <errno.h>
#include "DebugUtil.h"

// Flags for synchronization withe the scheduler
enum {
  CLEAR_SUSPENDFLAG 	= 0,
  TO_BE_SUSPENDED		= 1,
  TO_KEEP_ON_RUNNING	= 2,
  TO_BE_DISREGARDED	= 3,
  RESUMING_SUSPENDFLAG = 4
};

#define MAXIMUM_CONSECUTIVE_TIMESLICES_AS_NATIVE_WAITING 5

class ThreadManager;

/**
 * XXX Handles thread scheduling via pthread condition variables.
 */
class Scheduling {
 public:
  Scheduling();
  virtual ~Scheduling();

  /**
   * Increment #waitingInNativeVTFcode and attempt to acquire #sharedAccessKey.
   */
  void lockShareResourceAccessKey() {
    ++waitingInNativeVTFcode;
    pthread_mutex_lock(&sharedAccessKey);
  }

  /**
   * Attempt to acquire #sharedAccessKey with a fixed timeout \p abs_timeout.
   *
   * Return true if the caller was able to acquire the lock, or if \p
   * abs_timeout was reached. Return false if any other error code was returned.
   */
  bool lockShareResourceAccessKeyWithTimeout(const struct timespec *abs_timeout) {
    return pthread_mutex_timedlock(&sharedAccessKey, abs_timeout) != ETIMEDOUT;
  }

  /**
   * Release #sharedAccessKey.
   */
  void unlockShareResourceAccessKey() {
    pthread_mutex_unlock(&sharedAccessKey);
  }

  /**
   * Return true if #doNotSuspendUntil is less than the current real time.
   */
  bool isSuspendingAllowed();

  /**
   * Set #currentlyPolledForWhetherItIsNativeWaiting to true.
   */
  void flagThreadBeingCurrentlyPolledForNativeWaiting() {
    currentlyPolledForWhetherItIsNativeWaiting = true;
  }

  /**
   * Set #currentlyPolledForWhetherItIsNativeWaiting to false.
   */
  void clearThreadBeingCurrentlyPolledForNativeWaiting() {
    currentlyPolledForWhetherItIsNativeWaiting = false;
  }

  /**
   * Return #currentlyPolledForWhetherItIsNativeWaiting.
   */
  const bool& isThreadBeingCurrentlyPolledForNativeWaiting() const {
    return currentlyPolledForWhetherItIsNativeWaiting;
  }

  /**
   * Set #inVex to false.
   */
  void exitingVex() {
    inVex = false;
  }

  /**
   * If #awaken is false, then attempt to acquire #suspendLock.
   */
  void waitForThreadToBlock() {
    if (!awaken) {
      pthread_mutex_lock(&suspendLock);
    }
  }

  /**
   * Signal #condSuspendLock.
   */
  void signalBlockedThreadToResume() {
    pthread_cond_signal(&condSuspendLock);
  }

  /**
   * Release #suspendLock.
   */
  void allowSignalledThreadToResume() {
    pthread_mutex_unlock(&suspendLock);
  }

  /**
   * Increment #vtfInvocationsSinceLastResume.
   */
  void addVtfInvocationPoint() {
    ++vtfInvocationsSinceLastResume;
  }

  /**
   * Return #awaken.
   */
  bool const & wasAwaken() const {
    return awaken;
  }

  /**
   * Set #awaken to \p _awaken.
   */
  void setAwaken(const bool & _awaken) {
    awaken = _awaken;
  }

  /**
   * Set #awaken to true, #localManagerId to \p controllingManagerId and reset
   * both #vtfInvocationsSinceLastResume and consecutiveTimeslots to 0.
   */
  void onThreadResumeByManager(unsigned int &controllingManagerId) {
    awaken = true;
    vtfInvocationsSinceLastResume = 0;
    localManagerId = controllingManagerId;
    consecutiveTimeslots = 0;
  }

  /**
   * Attempt to acquire #suspendFlagLock, then set #suspendFlag to \p intention
   * and signal #condSuspendFlagLock before finally releasing #suspendFlagLock.
   *
   * The #suspendFlag of a thread determines whether it can be suspended or not.
   */
  void notifySchedulerForIntention(const short& intention);

  /**
   * Wait on condition variable #condSuspendFlagLock until #suspendFlag is no
   * longer CLEAR_SUSPENDFLAG, reset #suspendFlag then return #returnValue.
   *
   * The #suspendFlag of a thread determines whether it can be suspended or not.
   */
  short getSuspendingThreadIntention();

  /**
   * Wait on condition variable #condSuspendLock until #awaken is no longer
   * false.
   */
  void blockHereUntilSignaled();

  /**
   * Acquire #suspendFlagLock, set #suspendFlag to RESUMING_SUSPENDFLAG then
   * signal #condSuspendFlagLock.
   */
  void notifySchedulerThatThreadResumed();

  /**
   * Set #doNotSuspendUntil to the current real time plus 3 seconds.
   */
  void haltSuspendForAwhile();

  /**
   * Acquire #suspendLock.
   */
  void acquireThreadControllingLock();

  /**
   * Release #suspendLock.
   */
  void releaseThreadControllingLock();

  /**
   * Wait on condition variable #condSuspendFlagLock until #suspendFlag is no
   * longer CLEAR_SUSPENDFLAG, then reset #suspendFlag.
   *
   * The #suspendFlag of a thread determines whether it can be suspended or not.
   */
  void waitForResumingThread();

  /**
   * Set #awakeningFromJoin to \p value.
   */
  void setAwakeningFromJoin(const bool &value) {
    awakeningFromJoin = value;
    //		if (value) {
    //			++awakeningFromJoin;// = value;
    //		} else {
    //			// Can get below zero as all "waiting-end" calls set it to false, while it might not have been set
    //			if (awakeningFromJoin > 0) {
    //				--awakeningFromJoin;
    //			}
    //		}
  }

  /**
   * Increment #vtfInvocationsSinceLastResume.
   */
  void increaseVtfInvocationsSinceLastResume() {
    ++vtfInvocationsSinceLastResume;
  }

  /**
   * Return #awakeningFromJoin.
   */
  const bool& isAwakeningAfterJoin() const {
    return awakeningFromJoin;
  }

  /**
   * Return the current controlling thread manager for this thread.
   */
  ThreadManager* getThreadCurrentlyControllingManager() const {
    return threadCurrentlyControllingManager;
  }

  /**
   * Return the previous controlling thread manager for this thread.
   */
  ThreadManager* getPreviouslyControllingManager() const {
    return previouslyControllingManager;
  }

  /**
   * Return a pointer to the current controlling thread manager for this thread.
   */
  ThreadManager** getThreadCurrentlyControllingManagerPtr() {
    if (threadCurrentlyControllingManager != NULL) {
      return &threadCurrentlyControllingManager;
    } else {
      return NULL;
    }
  }

  /**
   * Set #threadCurrentlyControllingManager to \p
   * threadCurrentlyControllingManager.
   */
  void setThreadCurrentlyControllingManager(ThreadManager *threadCurrentlyControllingManager) {
    this->threadCurrentlyControllingManager = threadCurrentlyControllingManager;
  }

  /**
   * Release a thread from its controlling manager by setting
   * #waitingInNativeVTFcode to 0, #previousControllingManager to
   * #threadCurrentlyControllingManager and reset
   * #consecutiveTimesFoundNativeWaiting.
   */
  void releaseThreadFromItsControllingManager() {
    waitingInNativeVTFcode = 0;
    previouslyControllingManager = threadCurrentlyControllingManager;
    threadCurrentlyControllingManager = NULL;
    consecutiveTimesFoundNativeWaiting = MAXIMUM_CONSECUTIVE_TIMESLICES_AS_NATIVE_WAITING;
  }

  /**
   * Set #simulatingModel to true and release #suspendLock.
   *
   * Release the controlling key when a model is being simulated since the
   * scheduler can't stop it anymore. This only applies if the method body of
   * the model-described method should be executed.
   */
  void startSchedulerControlledModelSimulation();

  /**
   * Acquire #suspendLock and wait on condition variable #condSuspendLock until
   * #simulatingModel is false.
   */
  void blockUntilModelSimulationEnd();

  /**
   * Acquire #suspendLock, set #simulatingModel to false and signal
   * #condSuspendLock.
   *
   * Called when the model simulation ends, and synchronize with a thread that's
   * waiting or running real code.
   */
  void notifyModelSimulationEnd();

  /**
   * Set #simulatingModel to false.
   */
  void locklessNotifyModelSimulationEnd();

  /**
   * Set #defaultThreadManager to \p _defaultThreadManager.
   */
  static void setDefaultThreadManager(ThreadManager *_defaultThreadManager) {
    defaultThreadManager = _defaultThreadManager;
  }

  /**
   * Set #forceSuspend to false and return its previous value.
   */
  bool getAndResetForcedSuspendFlag() {
    bool temp = forceSuspend;
    forceSuspend = false;
    return temp;
  }

  /**
   * Return #forceSuspend.
   */
  bool const& isThreadSetToBeForcefullySuspended() const {
    return forceSuspend;
  }

  /**
   * Return #waitingInNativeVTFcode.
   */
  int const& getWaitingInNativeVTFcode() {
    return waitingInNativeVTFcode;
  }

  /**
   * Return #inVex.
   */
  bool isInVex() {
    return inVex;
  }

  /**
   * Set #vtfInvocationsSinceLastResume to \p _vtfInvocationsSinceLastResume.
   */
  void setVtfInvocationsSinceLastResume(const long long &_vtfInvocationsSinceLastResume) {
    vtfInvocationsSinceLastResume = _vtfInvocationsSinceLastResume;
  }

  /**
   * Set #waitingInNativeVTFcode to \p _value.
   */
  void setWaitingInNativeVTFcode(const int &_value) {
    waitingInNativeVTFcode = _value;
  }

  /**
   * Return #vtfInvocationsSinceLastResume.
   */
  long long const& getVtfInvocationsSinceLastResume() const {
    return vtfInvocationsSinceLastResume;
  }

  /**
   * Set #forceSuspend to true.
   */
  void forceThreadSuspend() {
    forceSuspend = true;
  }

 protected:
  /**
   * ID of the thread manager that controls this thread in the local ID array.
   */
  unsigned int localManagerId;

  /**
   * The current thread manager controlling this thread.
   */
  ThreadManager* threadCurrentlyControllingManager;

  /**
   * The previous thread manager controlling this thread.
   */
  ThreadManager* previouslyControllingManager;

  /**
   * Appears to be unused.
   */
  bool forceSuspend;

  /**
   * Appears to be unused, is set to false on construction, by method
   * #exitingVex and returned by #isInVex but not set by any method.
   */
  bool inVex;

  /**
   * TODO
   *
   * Used for coordinating with a joining thread since a thread exits VEX before
   * it actually dies and allowing blocked threads waiting to join to resume.
   */
  bool awakeningFromJoin;

  /**
   * Used with condition variable #condSuspendFlagLock to block this thread on
   * a scheduler intention.
   */
  short suspendFlag;

  /**
   * Used with condition variable #condSuspendFlagLock.
   */
  pthread_mutex_t suspendFlagLock;

  /**
   * Condition variable used to block this thread.
   */
  pthread_cond_t condSuspendFlagLock;

  /**
   * NAT_WAIT
   */
  long long vtfInvocationsSinceLastResume;

  /**
   * Used to avoid taking time while in VEX code.
   */
  int waitingInNativeVTFcode;				// used to avoid taking time while in VEX code

  /**
   * Used with condition variable #condSuspendLock to block this thread
   * unconditionally.
   */
  bool awaken;

  /**
   * Used with condition variable #condSuspendLock to block this thread while
   * simulating model-described methods.
   */
  bool simulatingModel;

  /**
   * Flag to ensure that only one scheduler is able to poll the current thread.
   */
  bool currentlyPolledForWhetherItIsNativeWaiting;

  /**
   * Used with condition variable #condSuspendLock.
   */
  pthread_mutex_t suspendLock;

  /**
   * Used for mutual exclusion of this thread's scheduling.
   */
  pthread_mutex_t sharedAccessKey;

  /**
   * Condition variable used to block this thread.
   */
  pthread_cond_t condSuspendLock;

  /**
   * Appears to be unused.
   */
  unsigned int consecutiveTimeslots;

  /**
   * Appears to be unused.
   */
  int consecutiveTimesFoundNativeWaiting;

  /**
   * Absolute real timestamp that this thread cannot suspend until.
   */
  long long doNotSuspendUntil;

  /**
   * The default thread manager.
   */
  static ThreadManager *defaultThreadManager;
};

#endif /* SCHEDULING_H_ */
