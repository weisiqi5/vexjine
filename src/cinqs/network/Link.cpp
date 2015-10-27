#include "network/Link.h"

#include "Debug.h"

Earth *Link::earth = new Earth();

Link::Link() {
	this->n = Network::nullNode ;
}


Link::Link( CinqsNode *n ) {
	this->n = n ;
	network = n->getNetwork();
}

void Link::setOwner( CinqsNode *n ) {
	owner = n ;
}

CinqsNode *Link::getOwner() {
	return owner ;
}//releasing resource

//
// Can be called by subclasses that have more than one target
// node
//
void Link::send( Customer *c, CinqsNode *n ) {
	Debug::traceMove( c, n ) ;
	network->enterNode(c, n);
}

//
// Can be overridden in subclasses that have more than one target
// node
//
void Link::move( Customer *c ) {
	send( c, n ) ;
}




