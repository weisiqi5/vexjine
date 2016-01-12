/*
 * MethodLog.h
 *
 *  Created on: 21 Apr 2011
 *      Author: root
 */

#ifndef METHODLOG_H_
#define METHODLOG_H_

#include "MethodCallInfo.h"

#include <stack>

/**
 * XXX Stores MethodCallInfo objects for a VEX monitored thread.
 */
class MethodLog
{
 public:
  /**
   * Constructor.
   */
  MethodLog();

  /**
   * Destructor.
   */
  virtual ~MethodLog();

  /**
   * Return #currentMethodInfo.
   */
  MethodCallInfo* getCurrentMethodInfo() {
    return currentMethodInfo;
  }

  /**
   * Set #currentMethodInfo to \p _newMethod.
   */
  void setCurrentMethodInfo(MethodCallInfo *_newMethod) {
    currentMethodInfo = _newMethod;
  }

  /**
   * Return #exitingMethodInfo.
   */
  MethodCallInfo* getExitingMethodInfo() {
    return exitingMethodInfo;
  }

  /**
   * Return #nextMethodInfoToUse.
   */
  unsigned int const& getNextMethodInfoToUse() {
    return nextMethodInfoToUse;
  }

  /**
   * Decrement #nextMethodInfoToUse by 1.
   */
  void decreaseNextMethodInfoToUse() {
    --nextMethodInfoToUse;
  }

  /**
   * Increment #nextMethodInfoToUse by 1.
   */
  void increaseNextMethodInfoToUse() {
    ++nextMethodInfoToUse;
  }

  /**
   * Return a pointer to #methodInfoStorage.
   */
  std::vector<MethodCallInfo*>* getMethodInfoStorage() {
    return methodInfoStorage;
  }

  /**
   * Push a new profiled method invoked by the thread this MethodLog is
   * associated with (\p callinfo) into #methodStack.
   */
  void pushMethodEntryEventIntoThreadMethodStack(MethodCallInfo *callinfo);

  /**
   * Return the method ID of #currentMethodInfo, or 0 if it is NULL.
   */
  int getCurrentMethodId();

  /**
   * Return #methodStack, creating a new instance if it is currently NULL.
   *
   * The constructor does not explicitly create #methodStack.
   */
  std::stack<MethodCallInfo*>* getThreadMethodStack();

 private:
  /**
   * Pointer to the most recently profiled method invoked by the thread this
   * MethodLog is associated with, that is, the parent of the method associated
   * with #exitingMethodInfo.
   */
  MethodCallInfo *currentMethodInfo;    // used to keep accounting of information regarding virtual accelerators

  /**
   * FIXME
   * This MethodCallInfo is created with default, nil values when constructing
   * a new MethodLog and is used by ThreadState, but it is never explicitly
   * set with information meaning the thread ID remains 0.
   *
   * Only scheduler/distributed/ThreadManagerServer.{h,cpp} makes calls to
   * exitingMethodInfo->setInfo().
   */
  MethodCallInfo *exitingMethodInfo;    // used to keep accounting of information regarding virtual accelerators

  /**
   * Stores all profiled methods invoked by the thread this MethodLog is
   * associated with, even after they've been removed from #methodStack.
   */
  std::vector<MethodCallInfo*> *methodInfoStorage;

  /**
   * Index of the next MethodCallInfo in #methodInfoStorage to use, which may
   * already exist in which case the existing MethodCallInfo is re-used.
   */
  unsigned int nextMethodInfoToUse;

  /**
   * Stack of all profiled methods invoked by the thread this MethodLog is
   * associated with.
   */
  std::stack<MethodCallInfo*>* methodStack;
};

#endif /* METHODLOG_H_ */
