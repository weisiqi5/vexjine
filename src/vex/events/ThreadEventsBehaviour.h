/*
 * ThreadEventsBehaviour.h
 *
 *  Created on: 18 Oct 2011
 *      Author: root
 */

#ifndef THREADEVENTSBEHAVIOUR_H_
#define THREADEVENTSBEHAVIOUR_H_

#include <vector>
#include <string>
#include <pthread.h>

// Forward declarations
class ThreadManagerRegistry;
class ThreadManager;
class VexThreadState;
class ThreadRegistry;
class EventLogger;
class ObjectRegistry;
class AggregateStateCounters;
class LockRegistry;
class PapiProfiler;

/**
 * XXX Handles thread related events.
 */
class ThreadEventsBehaviour
{
  public:
    /**
     * Constructor
     */
    ThreadEventsBehaviour(ThreadManagerRegistry *_managers, ThreadRegistry *_registry, EventLogger *_eventLogger, ObjectRegistry *_waitingOnObjectRegistry, LockRegistry *_lockRegistry, AggregateStateCounters *_aggregateStateTransitionCounters, const bool &_stackTraceMode, const bool &_usingVtfScheduler, PapiProfiler *_hpcProfiler);

    /**
     * Destructor
     */
    virtual ~ThreadEventsBehaviour();

    /**
     * Notify the ThreadRegistry that main is starting.
     */
    void onThreadMainStart(const long &threadId);

    /**
     * Called when a new thread is started.
     */
    void onStart(const long &threadId, const char *threadName);

    /**
     * Read names of threads from \p excludedThreadsFile that will not be simulated in VEX.
     */
    void registerThreadsToBeExcludedFromSimulation(char *excludedThreadsFile);

    /**
     * Specify the name of a thread that should not be simulated in VEX.
     */
    void addThreadToBeExcludedFromSimulation(const char *threadName);

    /**
     * Wrapped waiting calls must be invoked before and after an _existing_
     * waiting call is executed such that waiting happens externally of VEX.
     * Must be called just before a waiting call is executed in the profiled
     * program, and should have a matching #onWrappedWaitingEnd.
     *
     * For example:
     * \code{.unparsed}
     * VEX:     onWrappedWaitingStart()
     * Program: pthread_cond_wait()
     * VEX:     onWrappedWaitingEnd()
     * \endcode
     */
    void onWrappedWaitingStart();

    /**
     * Must be called just after a waiting call has been executed in the
     * profiled program, and should have a matching #onWrappedWaitingStart.
     */
    void onWrappedWaitingEnd();

    /**
     * TODO
     */
    void onWrappedTimedWaitingStart(const long &objectId, long &timeout, const int &nanoTimeout);

    /**
     * TODO
     */
    void onWrappedTimedWaitingEnd();

    /**
     * TODO
     */
    void onWrappedTimedWaitingInterrupt(const long &objectId);

    // Replacement waiting: calls to be invoked before releasing the lock of a waiting call to be REPLACED: waiting happens within VEX

    // This method should be called before a timed-waiting-type method is replaced by a virtual-time wait within VEX.
    // Because the mutex lock of the original wait will have to be released,
    // this call adds the id of the mutex in the registry of waiting objects, before this happens
    // This allows VEX to use the original semantics of the program to guarantee mutual exclusion
    // between a notifying and a waiting thread
    /**
     * Must be called before a timed-waiting-type method is replaced by a virtual time wait within VEX.
     */
    bool beforeReleasingLockOfAnyReplacedWaiting(const long &objectId, const bool &updateLockRegistry);
    bool beforeReleasingLockOfAnyReplacedWaiting(pthread_mutex_t * mutex, const bool &updateLockRegistry);

    /**
     *
     */
    bool onReplacedWaiting(const long &objectId, const bool &updateLockRegistry);
    bool onReplacedTimedWaiting(const long &objectId, const long &timeout, const int &nanoTimeout, const bool &updateLockRegistry);

    bool onSignallingOnObject(const long &objectId);
    bool onBroadcastingOnObject(const long &objectId);

    void onRequestingLock(const long &objectId);
    void onReleasingLock(const long &objectId);

    bool onSleep(const long &timeout, const int &nanoTimeout);
    bool onInterrupt(const long &threadId);


    // Used for explicit locking - overloaded to allow easy integration with POSIX synchronization primitives in JaVex (integration with Java) - not used without
    bool beforeTimedWaitingOn(pthread_mutex_t * mutex, const bool &updateLockRegistry);
    bool onTimedWaitingOn(pthread_mutex_t * mutex, const long &timeout, const int &nanoTimeout, const bool &updateLockRegistry);
    bool onReplacedWaiting(pthread_mutex_t * mutex, const bool &updateLockRegistry);
    bool onReplacedTimedWaiting(pthread_mutex_t * mutex, const long &timeout, const int &nanoTimeout, const bool &updateLockRegistry);
    bool onSignallingOnObject(pthread_mutex_t * mutex);
    bool onBroadcastingOnObject(pthread_mutex_t *mutex);
    void onRequestingLock(pthread_mutex_t * mutex);
    void onReleasingLock(pthread_mutex_t * mutex);


    void beforeCreatingThread(long threadToBeSpawnedApplicationLanguageId);
    void afterCreatingThread();

    void onJoin(const long &joiningThreadId);

    // Explicit calls mean that the code will block externally of VEX, but VEX will note the thread state changes
//    void onExplicitWaitingStart();
//    void onExplicitWaitingEnd();

//    void onExplicitTimedWaitingStart(const long &objectId, const long &timeout);


    void onPark(const bool &isAbsoluteTimeout, const long &timeout);
    void onParked();

    void onUnpark(const long &unparkingThreadId);

    void onInteractionPoint();
    void onYield();

    void onBackgroundLoadExecutionStart();                                    // let VEX know that a background process (supposedly CPU occupying) is about to start - its duration is measured in wallclock time
    void onBackgroundLoadExecutionEnd();                                    // let VEX know that a background process (supposedly CPU occupying) has finished - its duration is measured in wallclock time
    void onBackgroundLoadExecutionEndAt(const long long &executionDuration);// let VEX know that a background process (supposedly CPU occupying) has finished - its (possibly virtual) duration is provided by argument executionDuration

    void onEnd();

    long long getTime();
    void ensureThreadIsNotInNativeWaitingStateWhenEnteringVex();
//    bool ensureThreadIsNotInNativeWaitingStateWhenEnteringVex(VexThreadState *state, const long long &startingTime);
    ThreadManager *getCurrentlyControllingManagerOf(VexThreadState *state);


    void haltSuspendForAwhile();

    void beforeReinstrumentation();
    void afterReinstrumentation();

    void setProfilerType(EventLogger *_eventLogger, const bool &_stackTraceMode) {
        eventLogger = _eventLogger;
        stackTraceMode = _stackTraceMode;
    }

protected:
    long ensureTimeoutIsLessThanMax(const long &timeout, const int &nanoTimeout);
    bool isThreadInExcludedThreadNamesList(const char *threadName);

    void onLocklessReleasingLock(VexThreadState *state, const long &lockId);
    void onLocklessRequestingLock(VexThreadState *state, const long &lockId);

    bool onSignallingOneThreadOnObject(const long &objectId, ThreadManager *stateManager, const long long &interruptionTime);
    bool onSignallingAllThreadsOnObject(const long &objectId, ThreadManager *stateManager, const long long &interruptionTime);

    bool interruptThread(VexThreadState *stateOfThreadToInterrupt, ThreadManager *stateManager, const long long &interruptionTime, bool isThreadInterruptCall);
    bool interruptThread(const long &interruptedThreadId, ThreadManager *stateManager, const long long &interruptionTime, bool isThreadInterruptCall);


    ThreadManagerRegistry *managers;
    EventLogger *eventLogger;
    ThreadRegistry *registry;

    ObjectRegistry *waitingOnObjectRegistry;        // a registry of threads waiting on an Object (used for wait - notify sync)
    LockRegistry *lockRegistry;                    // a registry of threads requesting or holding locks (monitors)

    AggregateStateCounters *aggregateStateTransitionCounters;
    bool stackTraceMode;
    bool usingVtfScheduler;
    PapiProfiler *hpcProfiler;
    std::vector<std::string> excludedThreadsList;
};

#endif /* THREADEVENTSBEHAVIOUR_H_ */
