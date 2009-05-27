/*
 *  Directional2DEnvironment.h
 *  hog2
 *
 *  Created by Nathan Sturtevant on 2/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef DIRECTIONAL2DENVIRONMENT
#define DIRECTIONAL2DENVIRONMENT

#include <stdint.h>
#include <iostream>
#include "Map.h"
#include "SearchEnvironment.h"
#include "UnitSimulation.h"
#include "ReservationProvider.h"
#include "BitVector.h"

#include <cassert>


//struct heading {
//public:
//	float x;
//	float y;
//	void normalize()
//	{
//		double length = sqrt(x * x + y * y);
//		if (length != 0)
//		{  x /= length; y /= length; }
//		else
//		{  x = 0; y = 0; }
//	}		
//	void add(heading &h)
//	{ x += h.x; y += h.y; normalize(); }
//	void turn(float angle) // in radians
//	{
//		float ang = atan(y/x);
//		ang+=angle;
//		x = cos(angle);
//		y = sin(angle);
//	}
//};
//
//struct xySpeedHeading {
//public:
//	xySpeedHeading() {}
//	xySpeedHeading(float _x, float _y) :x(_x), y(_y) {}
//	uint8_t speed;
//	heading h;
//	float x;
//	float y;
//};
//
//struct xySpeedHeading {
//public:
//	xySpeedHeading() {}
//	xySpeedHeading(float _x, float _y) :x(_x), y(_y) {}
//	uint8_t speed;
//	heading h;
//	float x;
//	float y;
//};


// action
struct deltaSpeedHeading {
public:
	int8_t turn; // -3...3
	int8_t speed; // +- some amount
};

// state
struct xySpeedHeading {
public:
	xySpeedHeading() {}
	xySpeedHeading(float _x, float _y) :x(_x), y(_y), speed(0), rotation(0) { }
	float x;
	float y;
	int8_t speed;
	int8_t rotation;
};

enum model {
	kHumanoid=0,
	kVehicle=1,
	kTank=2
};

enum heuristicType {
	kOctileHeuristic,
	kPerimeterHeuristic,
	kExtendedPerimeterHeuristic
};

class GoalTester {
public:
	virtual bool goalTest(const xySpeedHeading &i1) const = 0;	
};

class Directional2DEnvironment : public SearchEnvironment<xySpeedHeading, deltaSpeedHeading>
{
public:
	Directional2DEnvironment(Map *m);
	virtual ~Directional2DEnvironment();
	void GetSuccessors(xySpeedHeading &nodeID, std::vector<xySpeedHeading> &neighbors) const;
	void GetActions(xySpeedHeading &nodeID, std::vector<deltaSpeedHeading> &actions) const;
	deltaSpeedHeading GetAction(xySpeedHeading &s1, xySpeedHeading &s2) const;
	virtual void ApplyAction(xySpeedHeading &s, deltaSpeedHeading dir) const;
	virtual void UndoAction(xySpeedHeading &s, deltaSpeedHeading dir) const;
	virtual OccupancyInterface<xySpeedHeading,deltaSpeedHeading> *GetOccupancyInfo() { return 0; }

	virtual bool InvertAction(deltaSpeedHeading &a) const;
	
	virtual double HCost(xySpeedHeading &node1, xySpeedHeading &node2);
	virtual double GCost(xySpeedHeading &node1, xySpeedHeading &node2);
	virtual double GCost(xySpeedHeading &node1, deltaSpeedHeading &act);

	void SetGoalTest(GoalTester *t) {test = t;}
	bool GoalTest(xySpeedHeading &node, xySpeedHeading &goal);
	uint64_t GetStateHash(xySpeedHeading &node) const;
	uint64_t GetActionHash(deltaSpeedHeading act) const;
	virtual void OpenGLDraw() const;
	virtual void OpenGLDraw(const xySpeedHeading &l) const;
	virtual void OpenGLDraw(const xySpeedHeading& oldState, const xySpeedHeading &newState, float perc) const;
	virtual void OpenGLDraw(const xySpeedHeading &, const deltaSpeedHeading &) const;
	Map* GetMap() { return map; }
	void SetHeuristicType(heuristicType theType) { hType = theType; }
	virtual void GetNextState(xySpeedHeading &currents, deltaSpeedHeading dir, xySpeedHeading &news) const;	
private:
	bool Legal(xySpeedHeading &node1, deltaSpeedHeading &act) const;

	void BuildHTable();
	void BuildAStarTable();
	void BuildAngleTables();
	float mySin(int dir) const;
	float myCos(int dir) const;
	float LookupStateHash(xySpeedHeading &s);
	float LookupStateHeuristic(xySpeedHeading &s1, xySpeedHeading &s2);
	bool checkLegal;
	GoalTester *test;
	model motionModel;
	heuristicType hType;
protected:
	std::vector<std::vector<float> > hTable;
//	std::vector<std::vector<std::vector<float> > > hTable;
	std::vector<float> cosTable;
	std::vector<float> sinTable;
	Map *map;
};

typedef UnitSimulation<xySpeedHeading, deltaSpeedHeading, Directional2DEnvironment> DirectionSimulation;

static bool operator==(const xySpeedHeading &l1, const xySpeedHeading &l2)
{
	if (l1.speed != l2.speed)
		return false;
	if (l1.rotation != l2.rotation)
		return false;
	if ((floorf(l1.x) != floorf(l2.x)) && (!fequal(l1.x, l2.x)))
		return false;
	if ((floorf(l1.y) != floorf(l2.y)) && (!fequal(l1.y, l2.y)))
		return false;
	return true;
}

static bool operator==(const deltaSpeedHeading &l1, const deltaSpeedHeading &l2)
{
	if (l1.speed != l2.speed)
		return false;
	if (l1.turn != l2.turn)
		return false;
	return true;
}

static std::ostream& operator <<(std::ostream &out, const xySpeedHeading &loc)
{
	out << "(" << loc.x << ", " << loc.y << ")";
	out << "[" << (int)loc.speed << ":" << (int)loc.rotation << "]";
	return out;
}

static std::ostream& operator <<(std::ostream &out, const deltaSpeedHeading &loc)
{
	out << "{" << ((int)loc.turn) << "|" << ((int)loc.speed) << "}";
	return out;
}

#endif