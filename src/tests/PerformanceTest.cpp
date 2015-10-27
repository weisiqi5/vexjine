/*
 * PerformanceTest.cpp
 *
 *  Created on: 12 Nov 2011
 *      Author: root
 */

#include "PerformanceTest.h"
#include "AgentTester.h"
#include <cstdlib>

#include "../profiler/PapiProfiler.h"

PerformanceTest::PerformanceTest() {

}

PerformanceTest::~PerformanceTest() {

}

#define PERFORMANCE_TEST_METHOD_ID 101
// Classes to measure the overhead
void MethodsPerformanceTest::run(int loops) {
	for (int i = 0; i<loops; i++) {
		methodEventsBehaviour->afterMethodEntry(PERFORMANCE_TEST_METHOD_ID);
		methodEventsBehaviour->beforeMethodExit(PERFORMANCE_TEST_METHOD_ID);
	}
}

void IoPerformanceTest::run(int loops) {
	for (int i = 0; i<loops; i++) {
		methodEventsBehaviour->afterIoMethodEntry(4, 101010, false);
		methodEventsBehaviour->beforeIoMethodExit(4);
	}
}


// Loop-classes to show the effect of time compensation
double InstrLoopsTest::getNext(int i) {
	methodEventsBehaviour->afterMethodEntry(102);
	double temp = (double)(i * i) / (i+1);
	methodEventsBehaviour->beforeMethodExit(102);
	return temp;
}

void InstrLoopsTest::run(int loops) {
	int methodId = 101;
	eventLogger->registerMethod("loopingMethodPerformanceTest", methodId);
	methodEventsBehaviour->afterMethodEntry(methodId);
	e = 1.0;
	for (int i = 1; i<loops; i++) {
		e += getNext(i);
	}
	methodEventsBehaviour->beforeMethodExit(methodId);

}

double LoopsTest::getNext(int i) {
	double temp = (double)(i * i) / (i+1);
	return temp;
}

void LoopsTest::run(int loops) {
	e = 1.0;
	for (int i = 1; i<loops; i++) {
		e += getNext(i);
	}
}


static unsigned long next = 1;

// And the effect of different code to the compensation itself
double InstrLoopsTest2::getNext(int i) {
	methodEventsBehaviour->afterMethodEntry(102);
	next = next * 1103515245 + 12345;
	double temp = ((unsigned)(next/65536) % 32768) % i;
	methodEventsBehaviour->beforeMethodExit(102);
	return temp;
}

double LoopsTest2::getNext(int i) {
	next = next * 1103515245 + 12345;
	double temp = ((unsigned)(next/65536) % 32768) % i;
	return temp;
}

double InstrLoopsTest3::getNext(int i) {
	methodEventsBehaviour->afterMethodEntry(102);
	double temp = rand();
	methodEventsBehaviour->beforeMethodExit(102);
	return temp;
}

double LoopsTest3::getNext(int i) {
	double temp = rand();
	return temp;
}


int main(int argc, char **argv) {

	if (argc != 2 && argc != 3) {
		cout << "Syntax error: ./perftest <methods|io|loops[1-3]> [set to 1 to enable VEX|\"profiling_HPC\" for real time]" << endl;
		cout << "Wrong arguments: Select performance test among methods, io" << endl;
		return -1;
	}

	long long start = 0, end = 0;

	if (argc == 3 && strcmp(argv[2], "profiling_HPC") != 0) {
		char options[] = {"file=options"};
		VEX::initializeSimulator(options);

		eventLogger->registerMethod("main", 100);

		threadEventsBehaviour->onThreadMainStart(12521);
		methodEventsBehaviour->afterMethodEntry(100);
	}

	PerformanceTest *ptest;
	if (strcmp(argv[1], "methods") == 0) {
		ptest = new MethodsPerformanceTest();

	} else if (strcmp(argv[1], "io") == 0) {
		ptest = new IoPerformanceTest();

	} else if (strncmp(argv[1], "loops", 5) == 0) {

		// "loops*" && argc == 2 is real execution of loopstest
		if (argc == 2 || strcmp(argv[2], "profiling_HPC") == 0) {
			PapiProfiler *profiler = NULL;
			if (argc > 2 && strcmp(argv[2], "profiling_HPC") == 0) {
				profiler = new PapiProfiler(true);
				profiler->onThreadStart();
			}


			if (strcmp(argv[1], "loops1") == 0) {
				ptest = new LoopsTest();
			} else if (strcmp(argv[1], "loops2") == 0) {
				ptest = new LoopsTest2();
			} else {
				ptest = new LoopsTest3();
			}
			start = Time::getRealTime();
			ptest->run(100000000);
			end = Time::getRealTime();
			cout << (end-start)/1e9 << " sec (real)" << endl;
			if (profiler != NULL) {
				profiler->onThreadEnd("main");
				string profilerHpcFilename(argv[1]);
				profilerHpcFilename.append("_profiling_HPC.csv");
				profiler->getTotalMeasurements(profilerHpcFilename.c_str());
			}

		// "loops*" && argc == 3 is VEX execution of loopstest
		} else {
			if (strcmp(argv[1], "loops1") == 0) {
				ptest = new InstrLoopsTest();
			} else if (strcmp(argv[1], "loops2") == 0) {
				ptest = new InstrLoopsTest2();
			} else {
				ptest = new InstrLoopsTest3();
			}
			start = threadEventsBehaviour->getTime();
			ptest->run(10000000);
			end = threadEventsBehaviour->getTime();
			cout << (end-start)/1e9 << " sec (VEX)" << endl;
			methodEventsBehaviour->beforeMethodExit(100);
		}
		delete ptest;
		return 2;
	} else {
		cout << "Wrong value for test: acceptable values \"methods\" or \"io\"" << endl;
		return -1;
	}

	if (argc == 2) {
		// argc == 2 is real execution

		eventLogger->registerMethod("performanceTestMethod", PERFORMANCE_TEST_METHOD_ID);
		start = Time::getRealTime();
		ptest->run(10000000);
		end = Time::getRealTime();
		methodEventsBehaviour->beforeMethodExit(100);
		cout << (end-start)/10000000 << endl;

	} else {

		// argc == 3 is VEX execution
		int DEFAULT_ITERATIONS = 1000;
		int iterationsPerBlock = 1000;
		eventLogger->registerMethod("performanceTestMethod", PERFORMANCE_TEST_METHOD_ID);

		start = threadEventsBehaviour->getTime();		
		for (int i = 0; i<DEFAULT_ITERATIONS; i++) {
			ptest->run(iterationsPerBlock);			
		}
		end = threadEventsBehaviour->getTime();

		cout << (end-start)/(iterationsPerBlock * DEFAULT_ITERATIONS) << endl;

		methodEventsBehaviour->beforeMethodExit(100);
	}

	delete ptest;

	if (argc == 3 && strcmp(argv[2], "profiling_HPC") != 0) {
		threadEventsBehaviour->onEnd();
		VEX::endSimulator();
	}

	return 0;
}
