#ifndef TOOLS_EVENT_H
#define TOOLS_EVENT_H


class Event {

public:
	virtual void invoke() = 0;
	Event( double time ) ;
	double invokeTime() ;

	static int nextId ;

protected:
	int id;
	double _invokeTime;
};

#endif
