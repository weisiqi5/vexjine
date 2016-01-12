/*
 * MethodLog.cpp
 *
 *  Created on: 21 Apr 2011
 *      Author: root
 */

#include "MethodLog.h"

using namespace std;

MethodLog::MethodLog() {
  // Used for accounting of lost time.
  currentMethodInfo = NULL;
  exitingMethodInfo = new MethodCallInfo();

  methodStack = NULL;

  nextMethodInfoToUse = 0;

  // Never (unless in destructor) delete these MethodCallInfos, just update them
  methodInfoStorage = new vector<MethodCallInfo*>;
}

MethodLog::~MethodLog() {
  if (methodInfoStorage != NULL) {
    vector<MethodCallInfo*>::iterator iter;
    for (iter = methodInfoStorage->begin(); iter != methodInfoStorage->end(); iter++) {
      if ((*iter) != NULL) {
        delete (*iter);            // this cannot be NULL
        (*iter) = NULL;
      }
    }
    delete methodInfoStorage;    // none of them can be NULL
    methodInfoStorage = NULL;
  }

  if (exitingMethodInfo != NULL) {
    delete exitingMethodInfo;
    exitingMethodInfo = NULL;
  }
}

void MethodLog::pushMethodEntryEventIntoThreadMethodStack(MethodCallInfo *callinfo) {
  stack<MethodCallInfo*>* threadMethodStack = getThreadMethodStack();
  threadMethodStack->push(callinfo);
  currentMethodInfo = callinfo;
}

int MethodLog::getCurrentMethodId() {
  if (currentMethodInfo != NULL) {
    return currentMethodInfo->getMethodId();
  } else {
    return 0;
  }
}

stack<MethodCallInfo*>* MethodLog::getThreadMethodStack() {
  if (methodStack == NULL) {
    methodStack = new stack<MethodCallInfo*>;
    if (methodStack == NULL) {
      return NULL;
    }
  }
  return methodStack;
}
