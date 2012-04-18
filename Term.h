#ifndef _TERM_H_
#define _TERM_H_

#include "counted_ptr.h"

enum TermOperation {
	OP_PLUS,
	OP_MINUS,
	OP_DIV,
	OP_MUL,
	OP_MOD
};

enum TermType {
	T_IDENTIFIER,
	T_STATEMENT,
	T_NUMBER,
};

class Term {
public:	
	Term(TermType type) : type(type) {}
	virtual ~Term() {}
	
	TermType getType() {
		return type;
	}

private:
	TermType type;
};

class TermIdentNumber : public Term {
public:
	TermIdentNumber(int identName, TermType type) : Term(type), identName(identName) {
	}

	int getIdentName() {
		return identName;
	}

private:
	int identName;
};

class TermStatement : public Term {
public:
	TermStatement (counted_ptr<Term> pleft, counted_ptr<Term> pright, TermOperation op) 
		: Term(T_STATEMENT), op(op), right(pright), left(pleft) { }

	counted_ptr<Term> getLeft() {
		return left;
	}

	counted_ptr<Term> getRight() {
		return right;
	}

	TermOperation getOp() {
		return op;
	}

private:
	counted_ptr<Term> left;
	counted_ptr<Term> right;
	TermOperation op;
};


#endif