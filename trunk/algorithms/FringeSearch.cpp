/*
 *  FringeSearch.cpp
 *  hog
 *
 *  Created by Nathan Sturtevant on 1/19/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "FringeSearch.h"
#include "fpUtil.h"

static bool verbose = false;

inline double min(double i, double j)
{
	return (fless(i,j))?i:j;
}

inline double max(double i, double j)
{
	return (fgreater(i,j))?i:j;
}

FringeSearch::FringeSearch()
:searchAlgorithm()
{
	bpmx = false;
	currList = &list1;
	nextList = &list2;
	hp = 0;
}

path *FringeSearch::getPath(graphAbstraction *_aMap, node *from, node *to, reservationProvider *)
{
	initializeSearch(_aMap, from, to);
	
	if ((from == 0) || (to == 0) || (!aMap->pathable(from, to)) || (from == to))
		return 0;

	currList->push_back(from);
	currFLimit = getHCost(from);
	nextFLimit = MAXFLOAT;
	node *currOpenNode;
	while (currList->size() > 0)
	{
		currOpenNode = currList->back();
		currList->pop_back();
		if (currOpenNode == 0)
		{
			checkIteration();
			continue;
		}
		if (verbose) printf("Expanding %d\n", currOpenNode->getNum());
		
		if (fgreater(getFCost(currOpenNode), currFLimit))
		{
			if (verbose) printf("FCost %f above limit %f\n", getFCost(currOpenNode), currFLimit);
			nextFLimit = min(nextFLimit, getFCost(currOpenNode));
			nodesTouched++;
			addToOpenList2(currOpenNode);
			checkIteration();
			continue;
		}

		if (currOpenNode == to)
			break;
		
		addToClosedList(currOpenNode);
		nodesExpanded++;

		edge_iterator ei = currOpenNode->getEdgeIter();
		
		// iterate over all the children
		for (edge *e = currOpenNode->edgeIterNext(ei); e; e = currOpenNode->edgeIterNext(ei))
		{
			nodesTouched++;
			unsigned int which;
			if ((which = e->getFrom()) == currOpenNode->getNum()) which = e->getTo();
			node *neighbor = g->getNode(which);
			assert(neighbor != 0);

			if (onClosedList(neighbor))
			{
				if (verbose) printf("->Child %d on closed list\n", neighbor->getNum());
				updateCosts(neighbor, currOpenNode, e);
				continue;
			}
			else if (onOpenList(neighbor))
			{
				if (verbose) printf("->Child %d on open list\n", neighbor->getNum());
				updateCosts(neighbor, currOpenNode, e);
			}
			else // not on any list
			{
				addCosts(neighbor, currOpenNode, e);
				addToOpenList(neighbor);
				if (verbose) printf("->Child %d new to search f(%f) g(%f) h(%f)\n", neighbor->getNum(),
							 getFCost(neighbor), getGCost(neighbor), getHCost(neighbor));
			}
		}
		checkIteration();
	}
	//printf("Fringe %d nodes expanded, %d nodes touched\n", nodesExpanded, nodesTouched);
	return extractBestPath(to);
}

void FringeSearch::initializeSearch(graphAbstraction *aGraph, node *from, node *to)
{
	nodesTouched = 0;
	nodesExpanded = 0;
	nodesReopened = 0;
	nodesHPropagated = 0;
	list1.resize(0);
	list2.resize(0);
	closedList.resize(0);
	costTable.resize(0);
	aMap = aGraph;
	goal = to;
	g = aMap->getAbstractGraph(from->getLabelL(kAbstractionLevel));

	addCosts(from, 0, 0);
}

void FringeSearch::addToOpenList(node *n)
{
	n->key = currList->size();
	currList->push_back(n);
}

void FringeSearch::moveToOpenList1(node *n)
{
	if ((n->key < currList->size()) && ((*currList)[n->key] == n))
		return;
	if ((n->key < nextList->size()) && ((*nextList)[n->key] == n))
	{
		if (verbose) printf("Moved %d to current open list\n", n->getNum());
		(*nextList)[n->key] = 0;
		n->key = currList->size();
		currList->push_back(n);
	}
}

void FringeSearch::addToOpenList2(node *n)
{
	n->key = nextList->size();
	nextList->push_back(n);
}

void FringeSearch::addToClosedList(node *n)
{
	n->key = closedList.size();
	closedList.push_back(n);
}

bool FringeSearch::onClosedList(node *n)
{
	if ((n->key < closedList.size()) && (closedList[n->key] == n))
		return true;
	return false;
}

bool FringeSearch::onOpenList(node *n)
{
	if ((n->key < list1.size()) && (list1[n->key] == n))
		return true;
	if ((n->key < list2.size()) && (list2[n->key] == n))
		return true;
	return false;
}

double FringeSearch::getFCost(node *n)
{
	if (n == 0)
		return 0;
	costs val;
	getCosts(n, val);
	return val.gCost+val.hCost;
}

double FringeSearch::getGCost(node *n)
{
	if (n == 0)
		return 0;
	costs val;
	getCosts(n, val);
	return val.gCost;
}

double FringeSearch::getHCost(node *n)
{
	if (n == 0)
		return MAXFLOAT;
	costs val;
	getCosts(n, val);
	return val.hCost;
}

void FringeSearch::setHCost(node *n, double val)
{
	unsigned long index = n->getLabelL(kTemporaryLabel);
	if ((index < costTable.size()) && (costTable[index].n == n))
	{
		costTable[index].hCost = val;
		return;
	}
	printf("setHCost Error: node %d not found!\n", n->getNum());
	assert(false);
}

void FringeSearch::getCosts(node *n, costs &val)
{
	unsigned long index = n->getLabelL(kTemporaryLabel);
	if ((index < costTable.size()) && (costTable[index].n == n))
	{
		val = costTable[index];
		return;
	}
	printf("Error: node %d not found!\n", n->getNum());
	assert(false);
}

void FringeSearch::addCosts(node *n, node *parent, edge *e)
{
	n->markEdge(e);
	costs val;
	val.n = n;
	if ((parent) && (e))
	{
		val.gCost = getGCost(parent)+e->getWeight();
		val.hCost = h(n, goal);
		n->setLabelL(kTemporaryLabel, costTable.size());
		costTable.push_back(val);

		if (fgreater(getHCost(parent)-e->getWeight(), h(n, goal)))
		{ // regular path max
			if (verbose) printf("Doing regular path max!\n");
			setHCost(n, getHCost(parent)-e->getWeight());
		}
		if (bpmx)
		{
			if (fgreater(val.hCost-e->getWeight(), getHCost(parent)))
			{ // reverse path max!
				if (verbose) printf("-> %d h value raised from %f to %f\n", parent->getNum(),
														getHCost(parent), getHCost(n)-e->getWeight());
				setHCost(parent, getHCost(n)-e->getWeight());
				if (verbose) printf("Doing reverse path max!\n");
				propagateHValues(parent, 2);
			}
		}
	}
	else {
		val.gCost = getGCost(parent);
		val.hCost = h(n, goal);
		n->setLabelL(kTemporaryLabel, costTable.size());
		costTable.push_back(val);
	}
}

void FringeSearch::updateCosts(node *n, node *parent, edge *e)
{
	if (fgreater(getGCost(n), getGCost(parent)+e->getWeight()))
	{
		n->markEdge(e);
		costs &val = costTable[n->getLabelL(kTemporaryLabel)];
		if (verbose) printf("Updated g-cost of %d from %f to %f (through %d) -- (%f limit)\n", n->getNum(),
												val.gCost, getGCost(parent)+e->getWeight(), parent->getNum(), currFLimit);
		val.gCost = getGCost(parent)+e->getWeight();
		propagateGValues(n);
		// I check the nextFLimit, because we might want to update it for this node
		nextFLimit = min(nextFLimit, getFCost(n));
	}
}

path *FringeSearch::extractBestPath(node *n)
{
	path *p = 0;
	edge *e;
	// extract best path from graph -- each node has a single parent in the graph which is the marked edge
	// for visuallization purposes, an edge can be marked meaning it will be drawn in white
	while ((e = n->getMarkedEdge()))
	{
		if (verbose) printf("%d <- ", n->getNum());
		
		p = new path(n, p);
		
		e->setMarked(true);
		
		if (e->getFrom() == n->getNum())
			n = g->getNode(e->getTo());
		else
			n = g->getNode(e->getFrom());
	}
	p = new path(n, p);
	if (verbose) printf("%d\n", n->getNum());
	return p;	
}

void FringeSearch::checkIteration()
{
	if (currList->size() == 0) // swap our lists!
	{
		nodeList *tmp = currList;
		currList = nextList;
		nextList = tmp;
		currFLimit = nextFLimit;
		nextFLimit = MAXFLOAT;
		if (verbose)
			printf("Beginning new iteration, flimit %f, %d items in q\n",
												currFLimit, (int)currList->size());
	}
}

// just figure out how/when to call this!
void FringeSearch::propagateHValues(node *n, int dist)
{
	if (dist == 0)
		return;
	nodesExpanded++;
	nodesHPropagated++;
	edge_iterator ei = n->getEdgeIter();
	
	// iterate over all the children
	for (edge *e = n->edgeIterNext(ei); e; e = n->edgeIterNext(ei))
	{
		nodesTouched++;
		unsigned int which;
		if ((which = e->getFrom()) == n->getNum()) which = e->getTo();
		node *neighbor = g->getNode(which);
		
		if (onClosedList(neighbor) || onOpenList(neighbor))
		{
			if (fless(getHCost(neighbor), getHCost(n)-e->getWeight())) // do update!
			{
				if (verbose) printf("%d h value raised from %f to %f\n", neighbor->getNum(),
														getHCost(neighbor), getHCost(n)-e->getWeight());
				setHCost(neighbor, getHCost(n)-e->getWeight());
				propagateHValues(neighbor, dist-1);
			}
		}
	}			
}

// just figure out how/when to call this!
void FringeSearch::propagateGValues(node *n)
{
	if (onClosedList(n))
		nodesReopened++;

	nodesExpanded++;
	edge_iterator ei = n->getEdgeIter();
	if (onOpenList(n) && (!fgreater(getFCost(n), currFLimit)))
		moveToOpenList1(n);
	
	// iterate over all the children
	for (edge *e = n->edgeIterNext(ei); e; e = n->edgeIterNext(ei))
	{
		nodesTouched++;
		unsigned int which;
		if ((which = e->getFrom()) == n->getNum()) which = e->getTo();
		node *neighbor = g->getNode(which);
		
		if ((onOpenList(neighbor) || onClosedList(neighbor)))// && (neighbor->getMarkedEdge() == e)
		{
			updateCosts(neighbor, n, e);
		}
	}
}


double FringeSearch::h(node *n1, node *n2)
{
	if (hp)
		return hp->h(n1->getNum(), n2->getNum());
	if ((n1->getNum()+n2->getNum())%2)
		return aMap->h(n1, n2);
	return 0;
}

