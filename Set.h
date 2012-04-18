#ifndef _SET_H_
#define _SET_H_

#include "Token.h"
#include <vector>
#include "counted_ptr.h"

enum SetOperation {
	UNION, //+
	INTERSECTION, //* (multiply characteristic functions of the sets)
	RELATIVE_COMPLEMENT //  \ */
};

enum SetType {
	SET_EMPTY,
	SET_IDENTIFIER,
	SET_STATEMENT,
	SET_ENUM,
	SET_RANGE
};

class Set { //Identifier
public:	
	Set() : type(SET_EMPTY) {}
	Set(SetType type) : type(type) {}
	virtual ~Set() {}
	
	SetType getType() {
		return type;
	}

private:
	SetType type;
};

class SetIdentifier : public Set {
public:
	SetIdentifier(int name) : Set(SET_IDENTIFIER), name(name) {
	}

	int getName() {
		return name;
	}

private:
	int name;
};

class SetList : public Set {
public:
	SetList(vector<int> & pNumbers, vector<int> & pIdentifiers) : Set(SET_ENUM) {
		numbers = pNumbers;
		identifiers = pIdentifiers;
	}

	vector<int> & getNumbers() {
		return numbers;
	}

	vector<int> & getIdentifiers() {
		return identifiers;
	}

private:
	vector<int> numbers;
	vector<int> identifiers;
};

class SetRange : public Set {
public:
	SetRange(int first, int last) : Set(SET_RANGE), first(first), last(last) {
	}

	int getFirst() {
		return first;
	}

	int getLast() {
		return last;
	}

private:
	int first, last;
};

class SetStatement : public Set {
public:
	SetStatement (counted_ptr<Set> pleft, counted_ptr<Set> pright, SetOperation op) 
		: Set(SET_STATEMENT), op(op), right(pright), left(pleft) { }

	counted_ptr<Set> getLeft() {
		return left;
	}

	counted_ptr<Set> getRight() {
		return right;
	}

	SetOperation getOp() {
		return op;
	}

private:
	counted_ptr<Set> left;
	counted_ptr<Set> right;
	SetOperation op;
};


#endif