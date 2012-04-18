#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include <cstdlib>
#include <string>
#include <vector>
#include "Set.h"
#include "counted_ptr.h"

enum VariableType{
	VAR_SET,
	SET_CONTENT,
	VAR_CONTENT
};

class Variable {
public:
	Variable(VariableType type) : type(type) {}
	
	VariableType getType() {
		return type;
	}

	string print() {
		switch (type) {
		case VAR_SET:		return "var set";
		case SET_CONTENT:	return "set content";
		case VAR_CONTENT:	return "var set content";
		default:			return "variable without type";
		}
	}

private:
	VariableType type;
};

class VariableSet : public Variable {
public:
	VariableSet() : Variable(VAR_SET), set(NULL) {	}

	VariableSet(counted_ptr<Set> set) : Variable(VAR_SET), set(set) {
		if (set->getType() == SET_IDENTIFIER) set = counted_ptr<Set>(NULL);
	}

	counted_ptr<Set> getSet() {
		return set;
	}
private:
	counted_ptr<Set> set;
};

class VariableContent : public Variable {
public:
	VariableContent() : Variable(VAR_CONTENT) {	}

	struct Koord {
		int x;
		int y;
	};

	void setKoord(int block, Koord koord) {
		koords[block] = koord;
	}

	bool defInBlock(int block) {
		return koords.count(block);
	}

	Koord getKoord(int block) {
		return koords[block];
	}

private:
	map<int, Koord> koords;
};

#endif