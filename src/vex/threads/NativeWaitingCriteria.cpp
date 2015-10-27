/*
 * NativeWaitingCriteria.cpp
 *
 *  Created on: 15 Aug 2011
 *      Author: root
 */

#include "NativeWaitingCriteria.h"
#include "Constants.h"
#include <cstring>

#ifdef USING_LIBUNWIND
#include <libunwind.h>
#endif

#include <cassert>

using namespace std;
short NativeWaitingCriteriaFactory::criteriaType = 0;
float NativeWaitingCriteriaFactory::cpuPercentageOfRealTimeLimitToRecogniseNativeWaiting = 0.01;

NativeWaitingCriteria *NativeWaitingCriteriaFactory::getCriteria(Scheduling *scheduling, Timers *timers) {
	switch (criteriaType) {
		case 1: return new NoNativeWaitingCriteria();
		case 2: return new StackTraceBasedCriteria();
		case 3: return new CombinedCriteria(timers, cpuPercentageOfRealTimeLimitToRecogniseNativeWaiting);
		default: return new VexCountersBasedCriteria(scheduling, timers, cpuPercentageOfRealTimeLimitToRecogniseNativeWaiting);
	}
}

void NativeWaitingCriteriaFactory::setCriteriaType(const char *type) {
	if (strcmp(type, "none") == 0) {
		criteriaType = 1;
	} else if (strcmp(type, "stackTrace") == 0) {
		criteriaType = 2;
	} else if (strncmp(type, "combined", 8) == 0) {
		criteriaType = 3;
	} else if (strncmp(type, "counters", 8) == 0) {
		criteriaType = 0;

	}

	if ((criteriaType == 0 || criteriaType == 3) && (strlen(type) >= 10 && type[8] == '_')) {
		float val = atof(&type[9]);
		if (val > 0 && val < 100) {
			cpuPercentageOfRealTimeLimitToRecogniseNativeWaiting = val;
		} else {
			cerr << "Warning: Illegal value for Native Waiting percent threshold " << val << endl;
		}
	}
}

NativeWaitingCriteria::NativeWaitingCriteria() {

}

NativeWaitingCriteria::~NativeWaitingCriteria() {

}


NoNativeWaitingCriteria::NoNativeWaitingCriteria() {

}

bool NoNativeWaitingCriteria::isNativeWaiting(const long long &currentRealTime) {
	return false;
}

bool NoNativeWaitingCriteria::isThreadStillBlockedInNativeWait(const long long &realTimeAtBeginningOfSignalHandler, const long long &cpuTimeAtBeginningOfSignalHandler) {
	return false;
}

NoNativeWaitingCriteria::~NoNativeWaitingCriteria() {

}

VexCountersBasedCriteria::VexCountersBasedCriteria(Scheduling *_threadSchedulingInfo, Timers *_threadTimingInfo, const float &_cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress) {
	threadSchedulingInfo = _threadSchedulingInfo;
	threadTimingInfo = _threadTimingInfo;
	cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress = _cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress;
}


bool VexCountersBasedCriteria::isThreadStillBlockedInNativeWait(const long long &realTimeAtBeginningOfSignalHandler, const long long &cpuTimeAtBeginningOfSignalHandler) {
	return ((cpuTimeAtBeginningOfSignalHandler - threadTimingInfo->getLastCPUTime()) < cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress * (realTimeAtBeginningOfSignalHandler - threadTimingInfo->getLastRealTimeInHandler()));
}


void printStackFrame() {
	bool result = false;
	unw_cursor_t    cursor;
	unw_context_t   context;

	unw_getcontext(&context);
	unw_init_local(&cursor, &context);
	const short maxStack = 10;
	unw_step(&cursor);	// skip signal handler
	unw_step(&cursor);
	unw_step(&cursor);
	short current_stack_depth = 3;

	while (unw_step(&cursor) > 0 && current_stack_depth++ < maxStack) {
		unw_word_t  offset;
		unw_word_t  pc;

		unw_get_reg(&cursor, UNW_REG_IP,  &pc);
		char fname[64];
		fname[0] = '\0';
		unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
		if (strcmp(fname, "pthread_cond_wait") == 0) {
			result = true;
		}

		char library[256];
		library[0] = '\0';
		sprintf(library, "%p", (void *)pc);
		cout << "authority " << library << " " << fname << " -> " << result << endl;
	}
	cout << endl;

}

bool VexCountersBasedCriteria::isNativeWaiting(const long long &realTime) {
	return threadTimingInfo->doesTheDifferenceBetweenCpuAndRealTimeIndicateLackOfProgress(realTime, cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress);

//	if (threadTimingInfo->doesTheDifferenceBetweenCpuAndRealTimeIndicateLackOfProgress(realTime, cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress)) {
//		cout << "================== isnativewaiting!!!" << endl;
//		printStackFrame();
//		return true;
		/*
		if (threadSchedulingInfo->getWaitingInNativeVTFcode() < MAX_TIMES_THAT_VEX_INTERNAL_LOCKS_ARE_HELD_BY_SCHEDULER) {
			if (threadSchedulingInfo->getVtfInvocationsSinceLastResume() == 0) {
				cout << "================== isnativewaiting!!!" << endl;
				printStackFrame();
				return true;

			} else {
				cout << "================== returning false because of setVtfInvocationsSinceLastResume " << threadSchedulingInfo->getVtfInvocationsSinceLastResume() << endl;
				threadSchedulingInfo->setVtfInvocationsSinceLastResume(0);
				printStackFrame();
			}
		} else {
			cout << "================== returning false because of setwaiting " << threadSchedulingInfo->getWaitingInNativeVTFcode() << endl;
			threadSchedulingInfo->setWaitingInNativeVTFcode(0);
			printStackFrame();
		}*/
//	}

//	return false;
}

VexCountersBasedCriteria::~VexCountersBasedCriteria() {

}




StackTraceBasedCriteria::StackTraceBasedCriteria() {

}


bool contains(char *haystack, const char *originalNeedle, const unsigned int &needleLength) {
	if (haystack[0] == '\0') {
		return false;
	}
	unsigned int found = 0;
	char *originalHayStack = haystack;
	char *needle = const_cast<char *>(originalNeedle);

	while (*haystack++ == *needle++ && ++found < needleLength);
	if (found == needleLength) {
		return true;
	} else {
		return contains(originalHayStack+1, originalNeedle, needleLength);
	}
}

bool StackTraceBasedCriteria::isNativeWaiting(const long long &currentRealTime) {
#ifdef USING_LIBUNWIND
	bool result = false;
	unw_cursor_t    cursor;
	unw_context_t   context;

	unw_getcontext(&context);
	unw_init_local(&cursor, &context);
	const short maxStack = 10;
	unw_step(&cursor);	// skip signal handler
	unw_step(&cursor);
	unw_step(&cursor);
	short current_stack_depth = 3;

	while (unw_step(&cursor) > 0 && current_stack_depth++ < maxStack) {
		unw_word_t  offset;
		unw_word_t  pc;

		unw_get_reg(&cursor, UNW_REG_IP,  &pc);
		char fname[64];
		fname[0] = '\0';
		unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
		if (strcmp(fname, "pthread_cond_wait") == 0) {
			result = true;
		}

		char library[256];
		library[0] = '\0';
		sprintf(library, "%p", (void *)pc);

		if (contains(library, "libvex", 6) || contains(library, "libJVMTIAgent", 13)) {
			return false;
		}
	}

	return result;
#else
	cerr << "Cannot use stack-trace based NW identification criteria without libunwind" << endl;
	assert(false);
	return false;
#endif
}

bool StackTraceBasedCriteria::isThreadStillBlockedInNativeWait(const long long &realTimeAtBeginningOfSignalHandler, const long long &cpuTimeAtBeginningOfSignalHandler) {
	return isNativeWaiting(realTimeAtBeginningOfSignalHandler);
}


StackTraceBasedCriteria::~StackTraceBasedCriteria() {

}



CombinedCriteria::CombinedCriteria(Timers *_threadTimingInfo, const float &_cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress) {
	threadTimingInfo = _threadTimingInfo;
	cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress = _cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress;
}


bool CombinedCriteria::isNativeWaiting(const long long &currentRealTime) {

	return threadTimingInfo->doesTheDifferenceBetweenCpuAndRealTimeIndicateLackOfProgress(currentRealTime, cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress)
#ifdef USING_LIBUNWIND
		&& StackTraceBasedCriteria::isNativeWaiting(currentRealTime);
#else
		;
#endif
}

bool CombinedCriteria::isThreadStillBlockedInNativeWait(const long long &realTimeAtBeginningOfSignalHandler, const long long &cpuTimeAtBeginningOfSignalHandler) {
	return ((cpuTimeAtBeginningOfSignalHandler - threadTimingInfo->getLastCPUTime()) < cpuTimePercentageOfRealTimeThatIsAcceptedAsProgress * (realTimeAtBeginningOfSignalHandler - threadTimingInfo->getLastRealTimeInHandler()))
#ifdef USING_LIBUNWIND
		&& StackTraceBasedCriteria::isNativeWaiting(realTimeAtBeginningOfSignalHandler);
#else
		;
#endif

}

CombinedCriteria::~CombinedCriteria() {

}







































































/*
bool VexCountersBasedCriteria::isNativeWaiting(const long long &realTime) {

	if (!threadSchedulingInfo->isInVex() && threadTimingInfo->doesTheDifferenceBetweenCpuAndRealTimeIndicateLackOfProgress(realTime)) {	// increased at most 8 times by scheduler (inside ThreadManager.cpp while finding next runnable threads)
//		long long differenceFromEstimatedRealTime = threadTimingInfo->getDifferenceBetweenCpuAndRealTime(realTime);

//		if (differenceFromEstimatedRealTime > 0) {

		if (threadSchedulingInfo->getWaitingInNativeVTFcode() < 8) {

			if (threadSchedulingInfo->getVtfInvocationsSinceLastResume() == 0) {
				long long totalLostTime = threadSchedulingInfo->getVtfInvocationsSinceLastResume() * Time::getBytecodeDelay();

				long long differenceFromEstimatedRealTime = threadTimingInfo->getDifferenceBetweenCpuAndRealTime(realTime);
				if (differenceFromEstimatedRealTime - totalLostTime > 0) {
					return true;
				} else {
//					cout << "returning false beacuse diff from ert = " << differenceFromEstimatedRealTime << " and totallost " << totalLostTime << endl;
					return false;
				}
			} else {
//				cout << "returning false because of setVtfInvocationsSinceLastResume " << threadSchedulingInfo->getVtfInvocationsSinceLastResume() << endl;
				threadSchedulingInfo->setVtfInvocationsSinceLastResume(0);
			}
		} else {
//			cout << "returning false because of setwaiting " << threadSchedulingInfo->getWaitingInNativeVTFcode() << endl;
			threadSchedulingInfo->setWaitingInNativeVTFcode(0);
		}
	} else {
//		cout << "returning false because of notenoughdifference " << threadTimingInfo->getDifferenceBetweenCpuAndRealTime(realTime) << endl;
	}

//	if (!threadSchedulingInfo->isInVex() && threadSchedulingInfo->getWaitingInNativeVTFcode() < 8) {	// increased at most 8 times by scheduler (inside ThreadManager.cpp while finding next runnable threads)
////		long long differenceFromEstimatedRealTime = threadTimingInfo->getDifferenceBetweenCpuAndRealTime(realTime);
//
//
////		if (differenceFromEstimatedRealTime > 0) {
//
//		if (threadTimingInfo->doesTheDifferenceBetweenCpuAndRealTimeIndicateLackOfProgress(realTime)) {
//
//			if (threadSchedulingInfo->getVtfInvocationsSinceLastResume() == 0) {
//				long long totalLostTime = threadSchedulingInfo->getVtfInvocationsSinceLastResume() * Time::getBytecodeDelay();
//
//				long long differenceFromEstimatedRealTime = threadTimingInfo->getDifferenceBetweenCpuAndRealTime(realTime);
//				if (differenceFromEstimatedRealTime - totalLostTime > 0) {
//					return true;
//				} else {
//					cout << "returning false beacuse diff from ert = " << differenceFromEstimatedRealTime << " and totallost " << totalLostTime << endl;
//					return false;
//				}
//			} else {
//				cout << "returning false because of setVtfInvocationsSinceLastResume " << threadSchedulingInfo->getVtfInvocationsSinceLastResume() << endl;
//				threadSchedulingInfo->setVtfInvocationsSinceLastResume(0);
//			}
//		}
//	} else {
//		cout << "returning false because of setwaiting " << threadSchedulingInfo->getWaitingInNativeVTFcode() << endl;
//		threadSchedulingInfo->setWaitingInNativeVTFcode(0);
//	}

	return false;
}

VexCountersBasedCriteria::~VexCountersBasedCriteria() {

}

StackTraceBasedCriteria::StackTraceBasedCriteria() {

}


bool contains(char *haystack, const char *originalNeedle, const unsigned int &needleLength) {
	if (haystack[0] == '\0') {
		return false;
	}
	unsigned int found = 0;
	char *originalHayStack = haystack;
	char *needle = const_cast<char *>(originalNeedle);

	while (*haystack++ == *needle++ && ++found < needleLength);
	if (found == needleLength) {
		return true;
	} else {
		return contains(originalHayStack+1, originalNeedle, needleLength);
	}
}

bool StackTraceBasedCriteria::isNativeWaiting(const long long &currentRealTime) {
	return isNativeWaiting();
}

bool StackTraceBasedCriteria::isNativeWaiting() {
#ifdef USING_LIBUNWIND
	bool result = false;
	unw_cursor_t    cursor;
	unw_context_t   context;

	unw_getcontext(&context);
	unw_init_local(&cursor, &context);
	const short maxStack = 10;
	unw_step(&cursor);	// skip signal handler
	unw_step(&cursor);
	unw_step(&cursor);
	short current_stack_depth = 3;

	while (unw_step(&cursor) > 0 && current_stack_depth++ < maxStack) {
		unw_word_t  offset;
		unw_word_t  pc;

		unw_get_reg(&cursor, UNW_REG_IP,  &pc);
		char fname[64];
		fname[0] = '\0';
		unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
		if (strcmp(fname, "pthread_cond_wait") == 0) {
			result = true;
		}

		char library[256];
		library[0] = '\0';
		sprintf(library, "%p", (void *)pc);

		if (contains(library, "libvex", 6) || contains(library, "libJVMTIAgent", 13)) {
			return false;
		}
	}

	return result;
#else
	cerr << "Cannot use stack-trace based NW identification criteria without libunwind" << endl;
	assert(false);
	return false;
#endif
}


StackTraceBasedCriteria::~StackTraceBasedCriteria() {

}




bool CombinedCriteria::isNativeWaiting(const long long &currentRealTime) {
	if (VexCountersBasedCriteria::isNativeWaiting(currentRealTime)) {
#ifdef USING_LIBUNWIND

		bool result = false;
		unw_cursor_t    cursor;
		unw_context_t   context;

		unw_getcontext(&context);
		unw_init_local(&cursor, &context);
		const short maxStack = 10;
		short current_stack_depth = 0;
		while (unw_step(&cursor) > 0 && current_stack_depth++ < maxStack) {
			unw_word_t  offset;
			unw_word_t  pc;

			unw_get_reg(&cursor, UNW_REG_IP,  &pc);
			char fname[64];
			fname[0] = '\0';
			unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);

			if (strcmp(fname, "pthread_cond_wait") == 0) {
				return true;
			}

			if (current_stack_depth > 2) {
				char library[256];
				library[0] = '\0';
				sprintf(library, "%p", (void *)pc);
				if (contains(library, "libvex", 6) == 0 || contains(library, "libJVMTIAgent", 13) == 0) {
					return false;
				}
			}
		}

		return result;
#else
		return true;
#endif
	}
	return false;
}

CombinedCriteria::~CombinedCriteria() {

}
*/
