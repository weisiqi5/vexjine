/*
 * Statistics.h
 *
 *  Created on: 21 Apr 2011
 *      Author: root
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <cstring>
#include <iostream>
#include "DebugUtil.h"

#define POSSIBLE_THREADSTATES 25

#if GDB_USAGE == 1
#define LOG_LAST_VEX_METHOD(state) \
state->setLastVexMethodInvoked(__FUNCTION__);

#define LOG_LAST_VEX_METHOD_TO_CURRENT_STATE \
VexThreadState *state = VexThreadState::getCurrentThreadState(); \
if (state != NULL) state->setLastVexMethodInvoked(__FUNCTION__);
#else
#define LOG_LAST_VEX_METHOD(state) ;
#define LOG_LAST_VEX_METHOD_TO_CURRENT_STATE ;
#endif

typedef unsigned int StateTransitionMatrix[POSSIBLE_THREADSTATES][POSSIBLE_THREADSTATES];

// Time monitoring position codes
enum {
	AFTER_ENTERING_METHOD = 1,
	BEFORE_EXITING_METHOD = 2,
	IN_SIGNAL_HANDLER = 3,
	UPDATING_INTERNAL = 4,
	BEFORE_SUSPENDING = 5,
	JUMPING_TO_CURRENT_TIME = 6
};

/**
 * XXX Used to store all state transitions for a thread, time spent in each
 * state, and invocation points.
 */
class Statistics {
 public:
  /**
   * Create a new Statistics object with default, nil values.
   */
	Statistics();
	virtual ~Statistics();

  /**
   * Logs a transition from state \p currentState to \p nextState in the
   * transition table #stateTransitions only if #COUNT_STATE_TRANSITIONS is set.
   */
	void logTransition(const short &currentState, const short &nextState) {
#if COUNT_STATE_TRANSITIONS == 1
		++stateTransitions[currentState][nextState];
#endif
	}

  /**
   * Logs a transition from state \p currentState to \p nextState in the
   * transition table #stateTransitions, and log the duration \p
   * durationOfLastState in #totalTimePerState.
   */
	void logTransition(const short &currentState, const short &nextState, long long durationOfLastState) {
		totalTimePerState[currentState] += durationOfLastState;
		logTransition(currentState, nextState);
	}

  /**
   * Return monitorPosition.
   */
	short const& getMonitorPosition() {
		return monitorPosition;
	}

  /**
   * Set #monitorPosition to \p _newPosition.
   */
	void setMonitorPosition(const short &_newPosition) {
		monitorPosition = _newPosition;
	}

  /**
   * Set #monitorPosition to 0.
   */
	void clearMonitorPosition() {
		monitorPosition = 0;
	}

  /**
   * Return #invocationPoints.
   */
	unsigned long getInvocationPoints() {
		return invocationPoints;
	}

  /**
   * Set #invocationPoints to \p points.
   */
	void setInvocationPoints(const unsigned long &points) {
		invocationPoints = points;
	}

  /**
   * Increment #invocationPoints.
   */
	void addInvocationPoints() {
		++invocationPoints;
	}

  /**
   * Return a pointer to the first entry in #totalTimePerState.
   */
	long long* getTotalTimePerState() {
		return &totalTimePerState[0];
	}

  /**
   * Return a pointer to the #stateTransitions matrix.
   */
	StateTransitionMatrix* getStateTransitionsMatrix() {
		return &stateTransitions;
	}

  /**
   * Set #forelastVEXMethodInvoked to #lastVEXMethodInvoked and
   * #lastVEXMethodInvoked to \p _lastMethod.
   */
	void setLastVexMethodInvoked(const char *_lastMethod) {
		strcpy(forelastVEXMethodInvoked, lastVEXMethodInvoked);
		strcpy(lastVEXMethodInvoked, _lastMethod);
	}

  /**
   * Reset #stateTransitions and #totalTimePerState.
   */
	void clearTransitionCounters();

  /**
   * Denotes the position from which the thread is making a time update call,
   * appears to be unused.
   */
	short monitorPosition;

	/**
	 * Stores the amount of virtual time spent in each thread state.
	 */
	long long totalTimePerState[POSSIBLE_THREADSTATES];

  /**
   * TODO
   */
	unsigned long invocationPoints;

  /**
   * 2D matrix that stores the number of transitions from one state to another.
   */
	StateTransitionMatrix stateTransitions;

  /**
   * Name of the VEX method invoked before the last, appears to be unused.
   */
	char forelastVEXMethodInvoked[64];

  /**
   * Name of the last VEX method invoked, appears to be unused.
   */
	char lastVEXMethodInvoked[64];
};

#endif /* STATISTICS_H_ */
