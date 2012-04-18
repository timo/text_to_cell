#ifndef _PARSER_H_
#define _PARSER_H_

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include "Token.h"
#include "StringTable.h"
#include "Lexer.h"
#include "Set.h"
#include "BasicData.h"

class Parser {
public:
	Parser(Lexer & lexer, StringTable & strTbl) : lexer(lexer), strTbl(strTbl) {
		token = lexer.next();
	}

	bool parseFile(CellFile & file) {
		if (token->getType() == NO_POINTER) {
			file.noPointer = true;
			next();
		} else file.noPointer = false;

		if (!parseHead(file.head)) return false;//error;
		
		while (token->getType() != EOFILE) {
			Block block;
			if (!parseBlock(block)) return false;
			file.blocks.push_back(block);
			if (block.getTurn90()) {
				Block b90;
				turn90(block, b90);
				file.blocks.push_back(b90);
			}
			if (block.getTurn180()) {
				Block b180;
				turn180(block, b180);
				file.blocks.push_back(b180);
			}
			if (block.getTurn270()) {
				Block b270;
				turn270(block, b270);
				file.blocks.push_back(b270);
			}
			if (block.getMirrorX()) {
				Block bx;
				mirrorX(block, bx);
				file.blocks.push_back(bx);
			}
			if (block.getMirrorY()) {
				Block by;
				mirrorY(block, by);
				file.blocks.push_back(by);
			}
		}
		return true;
	}

	string getError() {
		return error.str();
	}

private:
	Lexer & lexer;
	StringTable & strTbl;
	counted_ptr<Token> token;
	stringstream error;
	int cellX, cellY;

	void next() {
		token = lexer.next();
	}

	bool parseHead(Head & head) {
		Expression expr;
		while(token->getType() == IDENTIFIER) {
			// add rule to global variables
			parseExpression(expr);
			head.getExpressions().push_back(expr);
		}

		if (!(token->getType() == PICTURE)) {
			error << "Error: expected picture in the head instead got " << token->print() << endl;
			return false;
		}

		if (!parsePicture(head.getCell())) {
			error << "Error: the Picture in the head is not formatted right" << endl;
			return false;
		}

		cellX = head.getCell()->getWidth();
		cellY = head.getCell()->getHeight();
		
		while(token->getType() == IDENTIFIER) {
			// add rule to global variables
			parseExpression(expr);
			head.getExpressions().push_back(expr);
		}
		if (token->getType() == LINE) next();
		else error << "Error: no '___' after the head instead " << token->print() << endl;
		
		return true;
	}
	
	bool parseBlock(Block & block) {
		// Local Expressions could be added here
		Constraint cons;
		bool firstPic(false), secondPic(false); 
		while(true) {
			switch(token->getType()) {
			case PICTURE:	if (!firstPic) {
								int xMain(-1), yMain(-1);
								if (!parsePicture(block.getLeft(), & xMain, & yMain)) return false;// parse picture to left		
								if (xMain < 0 || yMain < 0) {
									error << "Error: on the left side of a block there has to be a '$' marking the top left of the mapped cell" << endl;
									return false;
								}
								block.setXY(xMain, yMain);
								firstPic = true;
							} else if (!secondPic) {
								if (!parsePicture(block.getRight())) return false;
								secondPic = true;
							} else {
								error << "Error: too many pictures inside a block" << endl;
								return false;
							}
							break;
			case ARROW:		next(); break;
			case TURN:		if (!parseTurn(block)) return false;
							break;
			case ELSE:		block.setElse(true);
							next(); break;
			case MIRROR_X:	block.setMirror(true, block.getMirrorY());
							next();
			case MIRROR_Y:	block.setMirror(block.getMirrorX(), true);
							next(); break;
			case NUMBER:
			case IDENTIFIER:	if (!parseConstraint(cons)) return false;
								block.getConstraints().push_back(cons);
								break;
			case LINE:		next();
			case EOFILE:	if (firstPic && secondPic) return true;
							error << "Error: block is not complete there is a picture missing" << endl;
							return false;
			default:		error << "Error: unexpected token inside a block " << token->print() << endl;
							next();

			}
		}
	}

	bool parsePicture(counted_ptr<Picture<counted_ptr<CellStatement>>> picture, int * xMain = NULL, int * yMain = NULL) {
		// set the size
		if (!(token->getType() == PICTURE)) {
			error << "Error: expected a picture instead found " << token->print() << endl;
			return false;
		}
		picture->setSize(static_cast<PictureToken *>(token.get())->getWidth()
					, static_cast<PictureToken *>(token.get())->getHeight());
		next();
		while(token->getType() != PICTURE_END) {
			if(token->getType() != CELL) {
				error << "Error: expected CellToken in parsePicture instead " << token->print() << endl;
				return false;
			}
			int x(static_cast<CellToken *>(token.get())->getX())
				, y(static_cast<CellToken *>(token.get())->getY());
			counted_ptr<CellStatement> cStatement(new CellStatement());
			next();
			if (token->getType() == MAIN_CELL) {
				next();
				if ((xMain != NULL) && (yMain != NULL)) {
					*xMain = x; *yMain = y;
				} else {
					error << "Error: There is a '$' in Picture that should not have a main cell" << endl;
				}
			}
			if (!parseCell(cStatement)) return false;
			picture->set(x, y, cStatement);
		}
		next();
		return true;
	}

	bool parseCell(counted_ptr<CellStatement> cStatement) {
		// parseCell now has to be called with token == first Token in the cell not CELL token
		counted_ptr<Set> k(NULL);
		switch(token->getType()) {
			case MINUS:
			case NUMBER:{ 
				counted_ptr<Term> t = parseTerm();
				if (!t.get()) return false;
				if (t->getType() == T_NUMBER) {
					int num = static_cast<TermIdentNumber*>(t.get())->getIdentName();
					cStatement->setContent(CELL_NUMBER, num, counted_ptr<Term>(NULL), k);
					return true;
				}
				// will  only get here if t is TermStatement since T_NUMBER is not possible
				if (token->getType() == IN) {
					next();
					k = parseSetStatement();
					if (!(k.get())) {
						error << "Error: error while parsing a set" << endl;
						return false;
					}
					cStatement->setContent(TERM_IN_SET, 0, t, k);
				} else cStatement->setContent(CELL_TERM, 0, t, k);
				return true;
			}
			case IDENTIFIER: {
				int id = static_cast<IdentifierToken*>(token.get())->getIdentity();
				counted_ptr<Term> t = parseTerm(); 
				if (!t.get()) {
					error << "Error: error while parsing a term" << endl;
					return false;
				}
				if (token->getType() == IN) {
					next();
					k = parseSetStatement();
					if (!(k.get())) {
						error << "Error: error while parsing a set" << endl;
						return false;
					}
				}
				if (t->getType() == T_STATEMENT) {
					if (k.get()) cStatement->setContent(TERM_IN_SET, 0, t, k);
					else cStatement->setContent(CELL_TERM, 0, t, k);
				} else if (k.get()) cStatement->setContent(IDENTIFIER_IN_SET, id, counted_ptr<Term>(NULL), k);
				else cStatement->setContent(CELL_IDENTIFIER, id, counted_ptr<Term>(NULL), k);
				return true;
			}
			case PICTURE_END:
			case CELL: 
				cStatement.get()->setContent(EMPTY,-1, counted_ptr<Term>(NULL), k); 
				return true;

			case IN:
				next();
				k = parseSetStatement();
				if (!k.get()) {
					error << "Error: error while parsing a set" << endl;
					return false;
				}
				cStatement.get()->setContent(SET_ONLY, -1, counted_ptr<Term>(NULL), k);
				return true;

			case NIN: // could be used possibly
			
			case LBRACE: {
				counted_ptr<Term> t = parseTerm();
				if (!t.get()){
					error << "Error: error while parsing a term" << endl;
					return false;
				}
				if (token->getType() == IN) {
					next();
					k = parseSetStatement();
					if (!(k.get())) {
						error << "Error: error while parsing a set" << endl;
						return false;
					}
					cStatement->setContent(TERM_IN_SET, 0, t, k);
				} else cStatement->setContent(CELL_TERM, 0, t, k);
				return true;
			}
			case RBRACE:

			case PICTURE:
			case QMARK:
			case EQUALS:
			case COMMA:
			case PLUS:
			case STAR:
			case SLASH:
			case LINE:
			case ARROW:
			case EQUALS_EQUALS:
			case LCURLY:
			case RCURLY:
			case EOFILE:
			case ERROR:
			default: 
				error << "Error: encountered an unexpected token while parsing the contents of a cell" << token->print() << endl;
				return false;
		}
	}
	
	bool parseExpression(Expression & expr) {
		counted_ptr<Set> left = parseSet();
		if (!(left.get())) {
			error << "Error: expected an identifier connected to a set at the beginning of an expression" << endl;
			return false;
		}
		if (left->getType() != SET_IDENTIFIER) {
			error << "Error: the set on the left side of an Expression should be variable" << endl;
			return false;
		}
		
		if (token->getType() != EQUALS) {
			error << "Error: expected = in expression instead got " << token->print() << endl;
			return false;
		}
		next();
		
		counted_ptr<Set> right = parseSetStatement();
		if (!(right.get())) {
			error << "Error: error while parsing a set" << endl;
			return false;
		}

		expr.setLeft(left);
		expr.setRight(right);
		
		return true;
	}

	counted_ptr<Set> parseSetStatement() {
		counted_ptr<Set> left = parseSet();
		return parseSetStatement(left);
	}

	counted_ptr<Set> parseSetStatement(counted_ptr<Set> left) {
		SetOperation op;
		switch(token->getType()) {
			case PLUS:
				op = UNION;
				break;
			case SLASH:				// right now - and / mean the same thing => Relative Complement
			case MINUS:
				op = RELATIVE_COMPLEMENT;
				break;
			case STAR:
				op = INTERSECTION;
				break;
			
			case RBRACE:
				next();
				return left;		// needed
					
			case ARROW:				// could mean something later on
			case EQUALS_EQUALS:
			case QMARK:
			case IN:
			case NIN:

			case LCURLY:			// would have to mean something if Set Set meant something
			case LBRACE:
									// case RCURLY:
			case IDENTIFIER: 
			case NUMBER: 

			default: return left;
		}
		next();
		counted_ptr<Set> right = parseSet();
		counted_ptr<Set> result = counted_ptr<Set>(new SetStatement(left, right, op));
		return parseSetStatement(result);
	}

	counted_ptr<Set> parseSet() {
		if (token->getType() == IDENTIFIER) {
			int ident(static_cast<IdentifierToken*>(token.get())->getIdentity());
			next();
			return counted_ptr<Set>(new SetIdentifier(ident));		
		} else if (token->getType() == LCURLY) {
			next();
			return parseSetLCURLY();
		} else if (token->getType() == LBRACE) {
			next();
			return parseSetStatement();
		} else {
			error << "Error: unexpected token while parsing a set expected '(', '{' or an identifier connected to a set instead got " << token->print() << endl;
			return counted_ptr<Set>(NULL);
		}
	}

	counted_ptr<Set> parseSetLCURLY() {
		vector<int> numbers;
		vector<int> identifiers;

		// First run through the while loop rolled out to enable Range Sets
		if (token->getType() == NUMBER || token->getType() == MINUS) {
			if (token->getType() == MINUS) {
				next();
				if (token->getType() != NUMBER) {
					error << "Error: a '-' can only preceed a number" << endl;
					return counted_ptr<Set>(NULL);
				}
				numbers.push_back(- static_cast<NumberToken *>(token.get())->getNumber());
			} else numbers.push_back(static_cast<NumberToken *>(token.get())->getNumber());
			
			next();
			if (token->getType() != COMMA) {
				if (token->getType() == RCURLY) {
					next();
					return counted_ptr<Set>(new SetList(numbers, identifiers));
				}
				error << "Error: expected '}' at the end of a set not " << token->print() << endl;
				return counted_ptr<Set>(NULL);
			}
			next();

			// Here we know if it is a Range Set
			if (token->getType() == DOT_DOT_DOT) {
				next();
				if (token->getType() == COMMA) next();
				if (token->getType() != NUMBER && token->getType() != MINUS) {
					error << "Error: expected a number after '...' not " << token->print() << endl;
					return counted_ptr<Set>(NULL);
				}
				if (token->getType() == MINUS) {
					next();
					if (token->getType() != NUMBER) {
						error << "Error: a '-' can only preceed a number" << endl;
						return counted_ptr<Set>(NULL);
					}
					numbers.push_back(- static_cast<NumberToken *>(token.get())->getNumber());
				} else numbers.push_back(static_cast<NumberToken *>(token.get())->getNumber());
				
				next();
				if (token->getType() != RCURLY) {
					error << "Error: expected '}' after a range set instead got " << token->print() << endl;
					return counted_ptr<Set>(NULL);
				}
				next();
				return counted_ptr<Set>(new SetRange(numbers[0], numbers[1]));
			} 
		}
		//*/


		while (token->getType() != RCURLY) {
			if (token->getType() == NUMBER) 
					numbers.push_back(static_cast<NumberToken *>(token.get())->getNumber());
			else if (token->getType() == MINUS) {
				next();
				if (token->getType() != NUMBER) {
					error << "Error: a '-' can only preceed a number" << endl;
					return counted_ptr<Set>(NULL);
				}
				numbers.push_back(- static_cast<NumberToken *>(token.get())->getNumber());

			} else if (token->getType() == IDENTIFIER) 
					identifiers.push_back(static_cast<IdentifierToken *>(token.get())->getIdentity());
			else {
				error << "Error: following '{' or ',' identifier or number expected not " << token->print() << endl;
				return counted_ptr<Set>(NULL);
			}
			next();
			if (token->getType() == COMMA) next();
			else if (token->getType() != RCURLY) {
				error << "Error: expected ',' instead got " << token->print() << endl;
				return counted_ptr<Set>(NULL);
			}
		}
		next();
		return counted_ptr<Set>(new SetList(numbers, identifiers));
	}

	bool parseTurn(Block & block) {
		do {
			next();
			if (token->getType() != NUMBER) {
				error << "Error: after turn one or more numbers are expected separated by ','" << endl;
				return false;
			}
			int t = static_cast<NumberToken*>(token.get())->getNumber();
			switch(t) {
				case 90:  block.setTurn(true, block.getTurn180(), block.getTurn270()); break;
				case 180: block.setTurn(block.getTurn90(), true, block.getTurn270()); break;
				case 270: block.setTurn(block.getTurn90(), block.getTurn180(), true); break;
				default:  {
							stringstream str;
							str << "Error: after turn expected numbers are 90, 180, 270 instead got " << t << endl;
							error << str.str();
							return false;
						  }
			}
			next();
		} while (token->getType() == COMMA);
		return true;
	}

	bool parseConstraint(Constraint & cons)  {
		cons.setLeft(parseTerm());
		if (!cons.getLeft().get()) {
			error << "Error: error while parsing the left term within a constraint" << endl;
			return false;
		}
		switch(token->getType()) {
			case EQUALS_EQUALS: cons.setOp(OP_EQ_EQ);		next(); break;
			case LESS:			cons.setOp(OP_LESS);		next(); break;
			case LESS_EQ:		cons.setOp(OP_LESS_EQ);		next(); break;
			case GREATER:		cons.setOp(OP_GREATER);		next(); break;
			case GREATER_EQ:	cons.setOp(OP_GREATER_EQ);	next(); break;
			case NOT_EQ:		cons.setOp(OP_NOT_EQ);		next(); break;
			
			case EQUALS:		cons.setOp(OP_EQ_EQ);		next();
				error << "= is no RelationalOperator asumed ==" << endl; 
				break;
			
			default: error << "Error: a RelationalOperator is expected within a constraint" << endl;
				return false;
		}
		cons.setRight(parseTerm());
		if (!cons.getRight().get()) {
			error << "Error: error while parsing the right term within a constraint" << endl;
			return false;
		}
		return true;
	}

	counted_ptr<Term> parseTerm() {
		counted_ptr<Term> left = parseAdditionTerm();
		while (token->getType() == PLUS || token->getType() == MINUS) {
			TermOperation op = (token->getType() == MINUS) ? OP_MINUS : OP_PLUS;
			next();
			counted_ptr<Term> right = parseAdditionTerm();
			left = counted_ptr<Term>(new TermStatement(left,right,op));
		}
		return left;
	}

	counted_ptr<Term> parseAdditionTerm() {
		counted_ptr<Term> left = parseFactorTerm();
		while (token->getType() == STAR || token->getType() == SLASH || token->getType() == PERCENT) {
			TermOperation op = (token->getType() == STAR) ? OP_MUL : (token->getType()==SLASH) ? OP_DIV : OP_MOD;
			next();
			counted_ptr<Term> right = parseFactorTerm();
			left = counted_ptr<Term>(new TermStatement(left,right,op));
		}
		return left;
	}

	counted_ptr<Term> parseFactorTerm() {
		if (token->getType() == IDENTIFIER) {
			int ident(static_cast<IdentifierToken*>(token.get())->getIdentity());
			next();
			return counted_ptr<Term>(new TermIdentNumber(ident, T_IDENTIFIER));		
		} else if (token->getType() == NUMBER) {
			int number(static_cast<NumberToken*>(token.get())->getNumber());
			next();
			return counted_ptr<Term>(new TermIdentNumber(number, T_NUMBER));
		} else if (token->getType() == MINUS) {
			next();
			if (token->getType() != NUMBER) {
				error << "Error: a '-' can only preceed a number" << endl;
				return counted_ptr<Term>(NULL);
			}
			int number(- static_cast<NumberToken*>(token.get())->getNumber());
			next();
			return counted_ptr<Term>(new TermIdentNumber(number, T_NUMBER));
		} else if (token->getType() == LBRACE) {
			next();
			counted_ptr<Term> temp = parseTerm();
			if (token->getType() != RBRACE) {
				error << "Error: expected a ) instead got a " << token->print() << endl;
				return temp;
			}
			next();
			return temp;
		} else {
			error << "Error: a term has to start with one of the following identifier, number or ( instead got " << token->print() << endl;
			return counted_ptr<Term>(NULL);
		}
	}
	
	void turn90(Block & in, Block & out) {
		copy(in, out);

		int left(ceil(((float)in.getX())/cellX)), 
			right(ceil(((float)in.getLeft()->getWidth() - in.getX())/cellX)), 
			up(ceil(((float)in.getY())/cellY)), 
			down(ceil(((float)in.getLeft()->getHeight() - in.getY())/cellY));
		
		out.getLeft()->setSize(cellX*(up+down), cellY*(left+right));
		out.setXY(up*cellX,(right-1)*cellY);

		int x0 = in.getX() - left * cellX;
		int y0 = in.getY() - up * cellY;
		for (int x = 0; x < left+right; x++)
			for (int y = 0; y < down+up; y++) {
				copy(in.getLeft(), x0+x*cellX, y0+y*cellY, out.getLeft(), y*cellX, (left+right-x-1)*cellY, cellX, cellY);
			}
		
		/*stringstream strstr;
		strstr << "parser" << in.getBlockIdent() << ".txt";
		ofstream outStream;
		outStream.open(strstr.str());

		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = in.getLeft();
		for (int i = 0; i < pic->getHeight(); i++) {
			for (int j = 0; j < pic->getWidth(); j++) {
				outStream << pic->get(j,i)->print();
			}
			outStream << endl;
		}
		outStream << endl;
		pic = out.getLeft();
		for (int i = 0; i < pic->getHeight(); i++) {
			for (int j = 0; j < pic->getWidth(); j++) {
				outStream << pic->get(j,i)->print();
			}
			outStream << endl;
		}
		outStream.close();*/
	}

	void turn180(Block & in, Block & out) {
		copy(in, out);

		int left(ceil(((float)in.getX())/cellX)), 
			right(ceil(((float)in.getLeft()->getWidth() - in.getX())/cellX)), 
			up(ceil(((float)in.getY())/cellY)), 
			down(ceil(((float)in.getLeft()->getHeight() - in.getY())/cellY));
		
		out.getLeft()->setSize(cellX*(left+right), cellY*(up+down));
		out.setXY((right-1)*cellX,(down-1)*cellY);

		int x0 = in.getX() - left * cellX;
		int y0 = in.getY() - up * cellY;
		for (int x = 0; x < left+right; x++)
			for (int y = 0; y < down+up; y++) {
				copy(in.getLeft(), x0+x*cellX, y0+y*cellY, out.getLeft(), (left+right-x-1)*cellX, (up+down-y-1)*cellY, cellX, cellY);
			}
	}

	void turn270(Block & in, Block & out) {
		copy(in, out);

		int left(ceil(((float)in.getX())/cellX)), 
			right(ceil(((float)in.getLeft()->getWidth() - in.getX())/cellX)), 
			up(ceil(((float)in.getY())/cellY)), 
			down(ceil(((float)in.getLeft()->getHeight() - in.getY())/cellY));
		
		out.getLeft()->setSize(cellX*(up+down), cellY*(left+right));
		out.setXY((down-1)*cellX,left*cellY);

		int x0 = in.getX() - left * cellX;
		int y0 = in.getY() - up * cellY;
		for (int x = 0; x < left+right; x++)
			for (int y = 0; y < down+up; y++) {
				copy(in.getLeft(), x0+x*cellX, y0+y*cellY, out.getLeft(), (up+down-y-1)*cellX, x*cellY, cellX, cellY);
			}
	}

	void mirrorX(Block & in, Block & out) {
		copy(in, out);

		int left(ceil(((float)in.getX())/cellX)), 
			right(ceil(((float)in.getLeft()->getWidth() - in.getX())/cellX)), 
			up(ceil(((float)in.getY())/cellY)), 
			down(ceil(((float)in.getLeft()->getHeight() - in.getY())/cellY));
		
		out.getLeft()->setSize(cellX*(left+right), cellY*(up+down));
		out.setXY((right-1)*cellX,up*cellY);

		int x0 = in.getX() - left * cellX;
		int y0 = in.getY() - up * cellY;
		for (int x = 0; x < left+right; x++)
			for (int y = 0; y < down+up; y++) {
				copy(in.getLeft(), x0+x*cellX, y0+y*cellY, out.getLeft(), (left+right-x-1)*cellX, y*cellY, cellX, cellY);
			}
	}

	void mirrorY(Block & in, Block & out) {
		copy(in, out);

		int left(ceil(((float)in.getX())/cellX)), 
			right(ceil(((float)in.getLeft()->getWidth() - in.getX())/cellX)), 
			up(ceil(((float)in.getY())/cellY)), 
			down(ceil(((float)in.getLeft()->getHeight() - in.getY())/cellY));
		
		out.getLeft()->setSize(cellX*(left+right), cellY*(up+down));
		out.setXY(left*cellX,(down-1)*cellY);

		int x0 = in.getX() - left * cellX;
		int y0 = in.getY() - up * cellY;
		for (int x = 0; x < left+right; x++)
			for (int y = 0; y < down+up; y++) {
				copy(in.getLeft(), x0+x*cellX, y0+y*cellY, out.getLeft(), x*cellX, (up+down-y-1)*cellY, cellX, cellY);
			}
	}

	void copy(Block & in, Block & out) {
		out.setRight(in.getRight());
		out.setElse(in.getElse());
		out.getConstraints() = in.getConstraints();
	}

	void copy(counted_ptr<Picture<counted_ptr<CellStatement>>> in, int xIn, int yIn, counted_ptr<Picture<counted_ptr<CellStatement>>> out, int xOut, int yOut, int width, int height) {
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
				out->set(xOut+x, yOut+y, in->get(xIn+x, yIn+y));
	}

};

#endif