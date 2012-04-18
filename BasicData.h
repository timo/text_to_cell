#ifndef _DATA_H_
#define _DATA_H_

#include <cstdlib>
#include <string>
#include <vector>
#include "Picture.h"
#include "Token.h"
#include "Set.h"
#include "counted_ptr.h"


class Expression {
public:
	Expression() { }

	Expression(counted_ptr<Set> pLeft, counted_ptr<Set> pRight) 
		: left(pLeft), right(pRight) {  }

	~Expression() {
	}

	counted_ptr<Set> getLeft() {
		return left;
	}

	counted_ptr<Set> getRight() {
		return right;
	}

	void setLeft(counted_ptr<Set> pLeft) {
		left = pLeft;
	}

	void setRight(counted_ptr<Set> pRight) {
		right = pRight;
	}

private:
	counted_ptr<Set> left;
	counted_ptr<Set> right;
};

enum RelationalOperator {
	OP_EQ_EQ,		// default
	OP_LESS,
	OP_LESS_EQ,
	OP_GREATER,
	OP_GREATER_EQ,
	OP_NOT_EQ
};

class Constraint {
public:
	Constraint() : op(OP_EQ_EQ) { }

	counted_ptr<Term> getLeft() {return left;}
	counted_ptr<Term> getRight() {return right;}
	void setLeft(counted_ptr<Term> l) {left = l;}
	void setRight(counted_ptr<Term> r) {right = r;}

	RelationalOperator getOp() {return op;}
	void setOp(RelationalOperator o) {op = o;}
private:
	counted_ptr<Term> left, right;
	RelationalOperator op;
};


enum CellStatementType {
	EMPTY,
	CELL_NUMBER,
	CELL_IDENTIFIER,
	SET_ONLY,
	IDENTIFIER_IN_SET,
	CELL_TERM,
	TERM_IN_SET
};

class CellStatement {
public:	
	CellStatement() : type(EMPTY) {}

	CellStatement(CellStatementType type, int identNumber, counted_ptr<Set> pSet) 
		: type(type), identNumber(identNumber), set(pSet){ }
	CellStatement(CellStatementType type, counted_ptr<Term> term, counted_ptr<Set> pSet) 
		: type(type), term(term), set(pSet){ }

	int getIdentNumber() {return identNumber;}	
	counted_ptr<Set> getSet() {return set;}
	counted_ptr<Term> getTerm() {return  term;}
	
	CellStatementType getType() {return type;}

	void setContent(CellStatementType pType, int pIdentNumber, counted_ptr<Term> pTerm, counted_ptr<Set> pSet) {
		type = pType;
		identNumber = pIdentNumber;
		set = pSet;
		term = pTerm;
	}

	string print() {
		switch(type) {
		case EMPTY:				return "[      empty      ]";
		case CELL_NUMBER:		return "[     number      ]";
		case CELL_IDENTIFIER:	return "[   identifier    ]";
		case SET_ONLY:			return "[    set only     ]";
		case IDENTIFIER_IN_SET: return "[identifier in set]";
		case TERM_IN_SET:		return "[   term in set   ]";
		case CELL_TERM:			return "[      term       ]";
		default: return "STRANGE";
		}
	}

private:
	CellStatementType type;
	int identNumber;
	counted_ptr<Set> set;
	counted_ptr<Term> term;
};


class Head {
public:
	Head() : cell(new Picture<counted_ptr<CellStatement>>(counted_ptr<CellStatement>(new CellStatement()))) {  }
	
	counted_ptr<Picture<counted_ptr<CellStatement>>> getCell() {
		return cell;
	}

	vector<Expression> & getExpressions() {
		return expressions;
	}

private:
	counted_ptr<Picture<counted_ptr<CellStatement>>> cell;
	vector<Expression> expressions;
};

class Block {
public:
	Block() 
	: left (new Picture<counted_ptr<CellStatement>>(counted_ptr<CellStatement>(new CellStatement())))
	, right(new Picture<counted_ptr<CellStatement>>(counted_ptr<CellStatement>(new CellStatement())))
	, turn90(false), turn180(false), turn270(false), e(false), mx(false), my(false) { 
		static int blockNbr = 0;
		blockIdent = blockNbr++;
	}

	counted_ptr<Picture<counted_ptr<CellStatement>>> getLeft() {
		return left;
	}
	
	counted_ptr<Picture<counted_ptr<CellStatement>>> getRight() {
		return right;
	}
	void setRight(counted_ptr<Picture<counted_ptr<CellStatement>>> nr) {
		right = nr;
	}

	int getBlockIdent(){
		return blockIdent;
	}

	int getX() {return xMain;}
	int getY() {return yMain;}
	void setXY(int x, int y) {
		xMain = x;
		yMain = y;
	}
	
	bool getTurn90()  {return turn90;}
	bool getTurn180() {return turn180;}
	bool getTurn270() {return turn270;}
	void setTurn(bool t90, bool t180, bool t270) {
		turn90 = t90; turn180 = t180; turn270 = t270;
	}

	bool getMirrorX() {return mx;}
	bool getMirrorY() {return my;}
	void setMirror(bool x, bool y) {
		mx = x; my = y;
	}

	bool getElse() {return e;}
	void setElse(bool elseBit) {e = elseBit;}

	vector<Constraint> & getConstraints() {return constraints;}
private:
	counted_ptr<Picture<counted_ptr<CellStatement>>> left;
	counted_ptr<Picture<counted_ptr<CellStatement>>> right;
	int xMain, yMain, blockIdent;
	bool turn90, turn180, turn270, e, mx, my;
	vector<Constraint> constraints;
};

struct CellFile {
	Head head;
	vector<Block> blocks;
	bool noPointer;
};


#endif