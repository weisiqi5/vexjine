/*
 * ThreadQueue.h: The priority queue that is used to store all runnable threads
 * and determine the next thread to be resumed by the VEX scheduler
 *
 *  Created on: 9 Apr 2010
 *      Author: nb605
 */
#ifndef THREADQUEUE_H_
#define THREADQUEUE_H_

#include <queue>
#include <pthread.h>
#include "ThreadState.h"
#include "AdaptiblePQueue.h"
#include "Logger.h"

#include <sstream>

class ThreadManager;

/**
 * Maintains a thread-safe queue of threads, using an access lock and
 * AdaptiblePQueue as the underlying data structure.
 */
class ThreadQueue {
 public:
  /**
   * Constructor.
   */
  ThreadQueue();

  /**
   * Destructor.
   */
  ~ThreadQueue();

  /**
   * Locks #mutex.
   */
  void lockMutex();

  /**
   * Unlocks #mutex.
   */
  void unlockMutex();

  /**
   * Insert \p state into the thread queue.
   */
  void push(VexThreadState *state);

  /**
   * Return the top of the thread queue without removing it.
   */
  VexThreadState *top();

  /**
   * Remove and return the top of the thread queue.
   */
  VexThreadState *getNext();


  /**
   * If \p state equals the top of the thread queue then pop the thread queue
   * and return \p state, otherwise just return the top of the thread queue.
   */
  VexThreadState *getNextIfEqualsElseReturnTop(VexThreadState *state, ThreadManager *requestingManager);

  /**
   * Remove and return the top of the queue from manager \p requestingManager.
   */
  VexThreadState *getNext(ThreadManager *requestingManager);

  /**
   * Return true if #threadsQueue is empty.
   */
  bool empty();

  /**
   * Return the size of #threadsQueue.
   */
  unsigned int size();

  /**
   * Find thread \p state in #threadQueue, then forward its virtual timestamp
   * to thread with the next highest virtual timestamp that is not currently
   * simulating a model.
   *
   * This is used to force the next thread in the queue to progress if the thread
   * \p state is blocked while executing a simulated model method. In this case,
   * allowing another thread to proceed resolves the deadlock.
   */
  void setThreadToNextBiggestErt(VexThreadState *state);

  /**
   * Return true if the virtual timestamp of the thread at the head of the queue
   * is smaller than \p timestamp.
   */
  bool isNextRunnableThreadBefore(const long long &timestamp);

  /**
   * Update the heap structure of the underlying priority queue data structure.
   */
  void update();

  /**
   * Erase thread \p state from the priority queue.
   */
  void erase(VexThreadState *state);

  /**
   * Print the contents of the priority queue to \p cout.
   */
  void print();

  /**
   * Return true if thread \p state is in the priority queue.
   */
  bool find(VexThreadState *state);

  /**
   * Get the suspended or native waiting thread with the largest timestamp
   * bigger than the timestamp of \p askingThread in the priority queue.
   */
  long long getHighestRunnableTime(VexThreadState *askingThread);	// used for yield

  /**
   * Set #queueLogger to \p logger.
   */
  void setLog(Log *logger) {
    queueLogger = logger;
  }

  /**
   * Used for debugging to print the stack trace of the queue's updating points
   */
  std::string getAllCallingMethodsUntil(short maxStack);

  /**
   * Invalidate all expired I/O predictions.
   */
  void invalidateExpiredIoPredictions(const long long &currentRealTime);

 private:
  /**
   * Recursively invalidate all expired I/O predictions.
   */
  void recursivelyUpdatePendingIoRequests(VexThreadState *state, const long long &currentRealTime);

  /**
   * Underlying data structure holding a priority queue of thread states.
   */
  AdaptiblePQueue<VexThreadState *, std::deque<VexThreadState *>, threadStatePtr_compare> *threadsQueue;

  /**
   * Lock for synchronisation.
   */
  pthread_mutex_t mutex;

  /**
   * Debugging log.
   */
  Log *queueLogger;

  /**
   * Unused.
   */
  long long highestRunnableTime;
};

#endif /*THREADQUEUE_H_*/
