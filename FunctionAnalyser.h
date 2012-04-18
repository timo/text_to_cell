#ifndef _FUNCTION_ANALYSER_H_
#define _FUNCTION_ANALYSER_H_

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include "Token.h"
#include "StringTable.h"
#include "Lexer.h"
#include "Set.h"
#include "BasicData.h"
#include "Variable.h"


bool cell_comp (CellStatement cs1, CellStatement cs2) {
	if (cs1.getType() < cs2.getType()) return true;
	else if (cs1.getType() > cs2.getType()) return false;
	else return cs1.getIdentNumber() < cs2.getIdentNumber();
}


class FunctionAnalyser {
public:
	FunctionAnalyser(StringTable & strTable, map<int, counted_ptr<Variable>> & varTable, string name) 
		: varTable(varTable), strTable(strTable), posSet(counted_ptr<Set>(new Set())), setLists(counted_ptr<vector<CellStatement>>()), instance(-1), varUsed(false) {
		outStream.open(name + "_analysis.txt");
		tableStream.open(name + ".table");
	}

	~FunctionAnalyser() {
		outStream.close();
		tableStream.close();
	}

	bool analyseFunction(CellFile & program) {
		finished = false;
		seriousError = false;

		prepare(program);
		while (!finished) {
			outStream << printInstance() << "          ";
			if (doTable) printTableInstance();
			// test instance
			testInstance(program);

			outStream << endl;
			generateInstance(); // get next instance
		}
		return !seriousError;
	}

	string getError() {
		return error.str();
	}

private:
	map<int, counted_ptr<Variable>> & varTable;
	StringTable & strTable;
	stringstream error;
	ofstream outStream, tableStream;

	int cellX, cellY;
	Picture<counted_ptr<Set>> posSet;
	
	Picture<counted_ptr<vector<CellStatement>>> setLists;
	int mainX, mainY;
	Picture<int> instance;
	bool finished, seriousError, doTable, vonNeumann;
	Picture<bool> varUsed;

	void generateInstance() {
		// change instance
		for (int x = 0; x < instance.getWidth(); x++)
			for (int y = 0; y < instance.getHeight(); y++) {
				if (instance.get(x,y) >= 0) {
					instance.set(x,y, instance.get(x,y) + 1);
					if (instance.get(x,y) >= setLists.get(modX(x),modY(y))->size()) instance.set(x,y,0);
					else return;
				}
			}
		finished = true;
	}

	void prepare(CellFile & program){
		// prepare posSet
		cellX = program.head.getCell()->getWidth();
		cellY = program.head.getCell()->getHeight();
		posSet.setSize(cellX, cellY);

		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = program.head.getCell();
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (pic->get(i,j)->getType() != EMPTY) {
					posSet.set(i, j, pic->get(i,j)->getSet());
				}
			}

		// prepare SetLists
		setLists.setSize(cellX, cellY);
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (pic->get(i,j)->getType() != EMPTY) {
					setLists.set(i, j, counted_ptr<vector<CellStatement>>(new vector<CellStatement>()));
					bool(*cell_pt)(CellStatement, CellStatement) = cell_comp;
					set<CellStatement, bool (*) (CellStatement, CellStatement)> to(cell_pt);
					set<CellStatement, bool (*) (CellStatement, CellStatement)>::iterator it;
					prepareSetList(posSet.get(i,j), to);
					for (it = to.begin(); it != to.end(); it++) {
						setLists.get(i,j)->push_back(*it);
					}
					if (setLists.get(i,j)->size() == 0) {
						seriousError = true;
						error << "Error: there is an empty set inside the header" << endl;
						outStream << "Error: error while initializing there is an empty set inside the header" << endl;
						tableStream << "Error: error while initializing there is an empty set inside the header" << endl;
						finished = true;
					}
				}
			}
		if (seriousError) return;

		// prepare instance
		int left(0), right(0), up(0), down(0);
		for (int i = 0; i < program.blocks.size(); i++) {
			left = max(left,	program.blocks[i].getX());
			up   = max(up,		program.blocks[i].getY());
			right= max(right,	program.blocks[i].getLeft()->getWidth()  - program.blocks[i].getX());
			down = max(down,	program.blocks[i].getLeft()->getHeight() - program.blocks[i].getY());
		}
		
		instance.setSize(left+right, up+down);

		mainX = left;
		mainY = up;

		for (int i = 0; i < program.blocks.size(); i++) {
			pic = program.blocks[i].getLeft();
			for (int x = 0; x < pic->getWidth(); x++)
				for (int y = 0; y < pic->getHeight(); y++) {
					if (pic->get(x,y)->getType() != EMPTY) instance.set(x - program.blocks[i].getX() + mainX, y - program.blocks[i].getY() + mainY, 0);
				}
		}

		// Could be needed if empty squares in solution
		for (int i = 0; i < program.blocks.size(); i++) {
			counted_ptr<Picture<counted_ptr<CellStatement>>> h = program.head.getCell();
			pic = program.blocks[i].getRight();
			for (int x = 0; x < cellX; x++)
				for (int y = 0; y < cellY; y++) {
					if (pic->get(x,y)->getType() == EMPTY && h->get(x,y)->getType() != EMPTY) instance.set(x+mainX, y+mainY, 0); 	
				}
		}
		prepareOutput(program);
		if (doTable) prepareTableVariables();
	}

	void prepareOutput(CellFile & program) {
		//prepare outStream
		for (int x = 0; x < instance.getWidth(); x++) 
			for (int y = 0; y < instance.getHeight(); y++)
				if (instance.get(x,y) >= 0) outStream << standard(x-mainX,2) << "/" << standard(y-mainY, 3) << "|";
		outStream << "          ";
		for (int x = 0; x < cellX; x++) 
			for (int y = 0; y < cellY; y++)
				if (program.head.getCell()->get(x,y)->getType() != EMPTY) outStream << standard(x,2) << "/" << standard(y, 3) << "|";
		outStream << "rules and notes" << endl << "_________________________________________________________________________________________" << endl;

		//prepare tableStream
		doTable = !(mainX > cellX || mainY > cellY || instance.getWidth() > mainX+2*cellX || instance.getHeight() > mainY+2*cellY);
		if (doTable) {
			int n = 1;
			for (int x = 0; x < cellX; x++) 
				for (int y = 0; y < cellY; y++) {
					if (setLists.get(x,y).get()) n *= setLists.get(x,y)->size();
				}
			tableStream << "n_states:" << n << endl;
			bool b = true;
			for (int x = 0; x < instance.getWidth(); x++) {
				for (int y = 0; y < instance.getHeight(); y++) 
					if (instance.get(x,y) >= 0) {
						if ((x < mainX || x >= mainX + cellX) && (y < mainY || y >= mainY+cellY)) {
							b = false;
							break;
						}
					}
				if (!b) break;
			}
			vonNeumann = b;
			tableStream << "neighborhood:" << ((b) ? "vonNeumann" : "Moore") << endl << "symmetries:none" << endl << endl;
		} else tableStream << "Used neighborhood is bigger than Moore neighborhood. This is not yet possible for this format.";
	}


	// has to be tested i am a little causiouss
	void prepareSetList(counted_ptr<Set> set1, set<CellStatement, bool (*) (CellStatement, CellStatement)> & to) {
		switch (set1->getType()) {
		case SET_IDENTIFIER:{
								counted_ptr<Set> set2 = static_cast<VariableSet*>(varTable[static_cast<SetIdentifier*>(set1.get())->getName()].get())->getSet();
								prepareSetList(set2, to);
								break;
							}
		case SET_ENUM:		{
								SetList * setL = static_cast<SetList *>(set1.get());
								for (int i = 0; i < setL->getNumbers().size(); i++) {
									to.insert(CellStatement(CELL_NUMBER, setL->getNumbers()[i], counted_ptr<Set>()));
								}
								for (int i = 0; i < setL->getIdentifiers().size(); i++) {
									to.insert(CellStatement(CELL_IDENTIFIER, setL->getIdentifiers()[i], counted_ptr<Set>()));
								}
								break;
							}
		case SET_RANGE:		{
								SetRange * setR =  static_cast<SetRange*>(set1.get());
								for (int i = setR->getFirst(); i <= setR->getLast(); i++) {
									to.insert(CellStatement(CELL_NUMBER, i, counted_ptr<Set>()));
								}
								break;
							}
		case SET_STATEMENT:	{
								SetStatement * setS = static_cast<SetStatement*>(set1.get());
								prepareSetList(setS->getLeft(), to);
								switch(setS->getOp()) {
								case UNION:				prepareSetList(setS->getRight(), to); break;
								case INTERSECTION:	{
														bool(*cell_pt)(CellStatement, CellStatement) = cell_comp;
														set<CellStatement, bool (*) (CellStatement, CellStatement)> locSet(cell_comp);
														prepareSetList(setS->getRight(), locSet);
														set<CellStatement, bool (*) (CellStatement, CellStatement)>::iterator it;
														for ( it=to.begin() ; it != to.end(); it++ ) {
															if (!locSet.count(*it)) to.erase(it);
														}
														break;
													}
								case RELATIVE_COMPLEMENT: 
													{
														bool(*cell_pt)(CellStatement, CellStatement) = cell_comp;
														set<CellStatement, bool (*) (CellStatement, CellStatement)> locSet(cell_comp);
														prepareSetList(setS->getRight(), locSet);
														set<CellStatement, bool (*) (CellStatement, CellStatement)>::iterator it;
														for ( it=locSet.begin() ; it != locSet.end(); it++ ) {
															if (to.count(*it)) to.erase(*it);
														}
														break;
													}
								}
							}
		}
	}

	int modX(int x) {
		int x1 = (x - mainX) % cellX;
		if (x1 < 0) x1 += cellX;
		return x1;
	}

	int modY(int y) {
		int y1 = (y - mainY) % cellY;
		if (y1 < 0) y1 += cellY;
		return y1;
	}

	CellStatement & get(int x, int y) {
		return (*setLists.get(modX(x), modY(y)))[instance.get(x,y)];
	}
	
	string printInstance() {
		stringstream strStream;
		for (int x = 0; x < instance.getWidth(); x++) 
			for (int y = 0; y < instance.getHeight(); y++)
				if (instance.get(x,y) >= 0) {
					CellStatement cell = get(x,y);
					if (cell.getType() == CELL_IDENTIFIER)
						strStream << standard(strTable.getString(cell.getIdentNumber()));
					else strStream << standard(cell.getIdentNumber());
					strStream << "|";
				}
		return strStream.str();
	}
	
	void testInstance(CellFile & file) {
		vector<int> vec;
		for (int i = 0; i < file.blocks.size(); i++) {
			if (testInstanceInBlock(file.blocks[i])) vec.push_back(i);
		}

		printResult(file, vec);

		if (vec.empty()) {
			error << printInstance() << "Warning: no result" << endl;
		} else if (vec.size() == 1) {
			outStream << vec[0];
		} else {
			outStream << vec[0];
			bool b = true;
			for (int i = 1; i < vec.size(); i++) {
				if (file.blocks[vec[i]].getElse()) {
					outStream << " (and " << vec[i] << ")";
				} else {
					outStream << " and " << vec[i];
					b = false;
				}
			}
			if (b) { 
				// fine since all later blocks are tagged else
			} else if (compareResults(file, vec)) {
				// multiple blocks trigger for this instance but, have the same result
				outStream << "    Warning: multiple Blocks triggered (same result but try to tag later blocks <else>)";
				error << printInstance() << "Warning: multiple Blocks triggered (same result but try to tag later blocks <else>)" << vec[0];
				for (int i = 1; i < vec.size(); i++) error << ", " << vec[i];
				error << endl;
			} else {
				// multiple blocks trigger for this instance even multiple different results
				outStream << "    Error: multiple Blocks triggered (try to tag later blocks <else>)";
				error << printInstance() << "Error: multiple Blocks triggered (try to tag later blocks <else>)" << vec[0];
				for (int i = 1; i < vec.size(); i++) error << ", " << vec[i];
				error << endl;
				// not that serious but if we were harsh seriousError = true;
			}
		}
		
		if (!vec.empty()) testResultLegal(file.blocks[vec[0]]);
	}

	bool testInstanceInBlock(Block & block) {
		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = block.getLeft();
		for (int x = 0; x < pic->getWidth(); x++) {
			int x1 = x - block.getX() + mainX;
			for (int y = 0; y < pic->getHeight(); y++) {
				int y1 = y - block.getY() + mainY;
				counted_ptr<CellStatement> cell = pic->get(x,y);
				switch (cell->getType()) {
				case EMPTY:				break;
				case CELL_NUMBER:		if (get(x1,y1).getType() != CELL_NUMBER 
											|| cell->getIdentNumber() != get(x1,y1).getIdentNumber()) return false;
										break;
				case IDENTIFIER_IN_SET:	if (!inSet(get(x1,y1), cell->getSet(), block)) return false;
				case CELL_IDENTIFIER:	if (varTable[cell->getIdentNumber()]->getType() == SET_CONTENT) {
											if (get(x1,y1).getType() != CELL_IDENTIFIER
												|| cell->getIdentNumber() != get(x1,y1).getIdentNumber()) return false;
										} else if (varTable[cell->getIdentNumber()]->getType() == VAR_CONTENT) {
											VariableContent::Koord k = static_cast<VariableContent *>(varTable[cell->getIdentNumber()].get())->getKoord(block.getBlockIdent());
											k.x += mainX - block.getX();
											k.y += mainY - block.getY();
											if ((k.x != x1 || k.y != y1)
												&& (get(x1,y1).getType() != get(k.x, k.y).getType() 
												|| get(x1,y1).getIdentNumber() != get(k.x, k.y).getIdentNumber())) return false;
										}
										break;
				case SET_ONLY:			if (!inSet(get(x1,y1), cell->getSet(), block)) return false;
										break;
				case TERM_IN_SET:		if (!inSet(get(x1,y1), cell->getSet(), block)) return false;
				case CELL_TERM:			if (get(x1,y1).getType() != CELL_NUMBER) return false;
										if (get(x1,y1).getIdentNumber() != computeTerm(cell->getTerm(), block)) return false;
										break;
				}
			}
		}

		// test Constraints
		for (int i = 0; i < block.getConstraints().size(); i++)
			if (!testInstanceForConstraint(block.getConstraints()[i], block)) return false;

		return true;
	}

	bool testInstanceForConstraint(Constraint & cons, Block & block) {
		int l, r;
		l = computeTerm(cons.getLeft(), block);
		r = computeTerm(cons.getRight(), block);
		switch(cons.getOp()) {
		case OP_EQ_EQ:		return l == r;
		case OP_LESS:		return l <  r;
		case OP_LESS_EQ:	return l <= r;
		case OP_GREATER:	return l >  r;
		case OP_GREATER_EQ:	return l >= r;
		case OP_NOT_EQ:		return l != r;
		}
	}

	int computeTerm(counted_ptr<Term> t, Block & block) {
		switch(t->getType()) {
		case T_NUMBER:		return static_cast<TermIdentNumber*>(t.get())->getIdentName();
		case T_IDENTIFIER:	{
								VariableContent::Koord k = static_cast<VariableContent*>(varTable[static_cast<TermIdentNumber*>(t.get())->getIdentName()].get())->getKoord(block.getBlockIdent());
								k.x += mainX - block.getX();
								k.y += mainY - block.getY();
								if (get(k.x, k.y).getType() != CELL_NUMBER) {
									error << "Error: a variable used in a constraint is no number" << endl;
									seriousError = true;
								}
								return get(k.x, k.y).getIdentNumber();
							}
		case T_STATEMENT:	{
								TermStatement* ts = static_cast<TermStatement*>(t.get());
								int l(computeTerm(ts->getLeft(), block)), r(computeTerm(ts->getRight(), block));
								switch(ts->getOp()) {
								case OP_PLUS:	return l + r;
								case OP_MINUS:	return l - r;
								case OP_DIV:	return l / r;
								case OP_MUL:	return l * r;
								case OP_MOD:	return l % r;
								}
							}
		}
	}

	void printResult(CellFile & file, vector<int> & vec) {	
		Picture<int> p = Picture<int>(-1);                         // used for table
		p.setSize(cellX, cellY);

		if (vec.empty()) {
			for (int x = 0; x < cellX; x++)
				for (int y = 0; y < cellY; y++) {
					if (instance.get(mainX+x,mainY+y) >= 0) {
						p.set(x,y,instance.get(mainX+x,mainY+y));  // used for table
						CellStatement cell = get(mainX+x, mainY+y);
						if (cell.getType() == CELL_IDENTIFIER)
							outStream << standard(strTable.getString(cell.getIdentNumber()));
						else outStream << standard(cell.getIdentNumber());
						outStream << "|";
					}
				}
		} else {

			Block & b = file.blocks[vec[0]];
			counted_ptr<Picture<counted_ptr<CellStatement>>> pic = b.getRight();
			for (int x = 0; x < cellX; x++)
				for (int y = 0; y < cellY; y++) {
					if (file.head.getCell()->get(x,y)->getType() != EMPTY) {
						CellStatement c1 = *(pic->get(x,y));
						if ((c1.getType() == CELL_IDENTIFIER) && (varTable[c1.getIdentNumber()]->getType() == VAR_CONTENT)) {
							VariableContent::Koord k = static_cast<VariableContent*>(varTable[c1.getIdentNumber()].get())->getKoord(b.getBlockIdent());
							k.x += mainX - b.getX();
							k.y += mainY - b.getY();
							c1 = get(k.x, k.y);
						} else if (c1.getType() == EMPTY) {
							c1 = get(mainX+x, mainY+y);
						} else if (c1.getType() == CELL_TERM) {
							c1 = CellStatement(CELL_NUMBER, computeTerm(c1.getTerm(), b), counted_ptr<Set>(NULL));
						}

						for (int i = 0; i < setLists.get(x,y)->size(); i++) {
							if (c1.getType() == setLists.get(x,y)->at(i).getType() && c1.getIdentNumber() == setLists.get(x,y)->at(i).getIdentNumber()) {
								p.set(x,y,i);
								break;
							}
						}
					
						if (c1.getType() == CELL_NUMBER) outStream << standard(c1.getIdentNumber()) << '|';
						else if (c1.getType() == CELL_IDENTIFIER) outStream << standard(strTable.getString(c1.getIdentNumber())) << '|';
					}
				}
		}

		if (doTable) {
			printTableCell(p);
			tableStream << endl;
		}
	}

	void testResultLegal(Block & b) {
		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = b.getRight();
		for (int x = 0; x < cellX; x++)
			for (int y = 0; y < cellY; y++) {
				if ((pic->get(x,y)->getType() == CELL_IDENTIFIER ||
					pic->get(x,y)->getType() == IDENTIFIER_IN_SET) &&
					varTable[pic->get(x,y)->getIdentNumber()]->getType() == VAR_CONTENT) {
						// tests if all variables used in the result are legal in there cells
						
						VariableContent::Koord k = static_cast<VariableContent*>(varTable[pic->get(x,y)->getIdentNumber()].get())->getKoord(b.getBlockIdent());
							
						k.x += mainX - b.getX();
						k.y += mainY - b.getY();

						if (get(k.x, k.y).getIdentNumber()==5 && b.getBlockIdent() == 2) {
							int Attention = 1;
						}

						if (!inSet(get(k.x, k.y), posSet.get(x, y), b)) {
							outStream << "    Error: this mapping is illegal it contains a variable which is initialized with illegal content for its cell";
							error << printInstance() << "    Error: this mapping is illegal it contains a variable which is initialized with illegal content for its cell" << endl;
							seriousError = true;
						}
				} else if (pic->get(x,y)->getType() == CELL_TERM || pic->get(x,y)->getType() == TERM_IN_SET) {
						//tests if all terms compute to something thats legal in their cells
								
						CellStatement temp(CELL_NUMBER, computeTerm(pic->get(x,y)->getTerm(), b), counted_ptr<Set>(NULL));
						if (!inSet(temp, posSet.get(x,y), b)) {
							outStream << "    Error: this mapping is illegal it contains a term that computes to an illegal number for its cell";
							error << printInstance() << "    Error: this mapping is illegal it contains a term that computes to an illegal number for its cell" << endl;
							seriousError = true;
						}
				}
			}
	}

	// assert v.size() > 1
	bool compareResults(CellFile & file, vector<int> & v) {
		CellStatement c0,c1;
		for (int x = 0; x < cellX; x++)
			for (int y = 0; y < cellY; y++) {
				c0 = *file.blocks[v[0]].getRight()->get(x,y);
				if (c0.getType() == CELL_IDENTIFIER && varTable[c0.getIdentNumber()]->getType() == VAR_CONTENT) {
					VariableContent::Koord k = static_cast<VariableContent*>(varTable[c0.getIdentNumber()].get())->getKoord(file.blocks[v[0]].getBlockIdent());
					k.x += mainX - file.blocks[v[0]].getX();
					k.y += mainY - file.blocks[v[0]].getY();
					c0 = get(k.x,k.y);
				} else if (c0.getType() == EMPTY && file.head.getCell()->get(x,y)->getType() != EMPTY) {
					c0 = get(mainX + x, mainY + y);
				} else if (c0.getType() == CELL_TERM) {
					c0 = CellStatement(CELL_NUMBER, computeTerm(c0.getTerm(), file.blocks[v[0]]), counted_ptr<Set>(NULL));
				}
				for (int i = 1; i < v.size(); i++) {
					c1 = *file.blocks[v[i]].getRight()->get(x,y);
					if (c1.getType() == CELL_IDENTIFIER &&
						varTable[c1.getIdentNumber()]->getType() == VAR_CONTENT) {
						VariableContent::Koord k = static_cast<VariableContent*>(varTable[c1.getIdentNumber()].get())->getKoord(file.blocks[v[i]].getBlockIdent());
						k.x += mainX - file.blocks[v[i]].getX();
						k.y += mainY - file.blocks[v[i]].getY();
						c1 = get(k.x,k.y);
					} else if (c1.getType() == EMPTY && file.head.getCell()->get(x,y)->getType() != EMPTY) {
						c1 = get(mainX + x, mainY + y);
					} else if (c1.getType() == CELL_TERM) {
						c1 = CellStatement(CELL_NUMBER, computeTerm(c1.getTerm(), file.blocks[v[i]]), counted_ptr<Set>(NULL));
					}
					if (c0.getType() != c1.getType() ||
						c0.getIdentNumber() != c1.getIdentNumber()) return false;
				}
			}
		return true;
	}

	bool inSet(CellStatement & cell, counted_ptr<Set> set, Block & block) {
		switch(set->getType()) {
			case SET_IDENTIFIER:{	
				VariableSet* vset = static_cast<VariableSet*>(varTable[static_cast<SetIdentifier*>(set.get())->getName()].get());
				return inSet(cell, vset->getSet(), block);
			}
			case SET_ENUM:		{	
				SetList * lset = static_cast<SetList*>(set.get());
				vector<int> vec = (cell.getType() == CELL_NUMBER) ? lset->getNumbers() : lset->getIdentifiers();
				for (int i = 0; i < vec.size(); i++) {
					if (vec[i] == cell.getIdentNumber()) return true;
				}
				vec = lset->getIdentifiers();
				for (int i = 0; i < vec.size(); i++) {
					if (varTable[vec[i]]->getType() == VAR_CONTENT) {
						VariableContent::Koord k = static_cast<VariableContent*>(varTable[vec[i]].get())->getKoord(block.getBlockIdent());
						k.x += mainX - block.getX();
						k.y += mainY - block.getY();
						if (cell.getType() == get(k.x,k.y).getType()
							&& cell.getIdentNumber() == get(k.x,k.y).getIdentNumber()) return true;
					}
				}
				return false;
			}
			case SET_RANGE:		{
				if (cell.getType() == CELL_NUMBER) {
					SetRange * rset = static_cast<SetRange*>(set.get());
					return (cell.getIdentNumber() >= rset->getFirst() &&
							cell.getIdentNumber() <= rset->getLast());
				} else return false;
			}
			case SET_STATEMENT:	{
				SetStatement * sets = static_cast<SetStatement*>(set.get());
				switch (sets->getOp())  {
					case UNION:					return inSet(cell, sets->getLeft(), block) || inSet(cell, sets->getRight(), block);
					case INTERSECTION:			return inSet(cell, sets->getLeft(), block) && inSet(cell, sets->getRight(), block);
					case RELATIVE_COMPLEMENT:	return inSet(cell, sets->getLeft(), block) && !inSet(cell, sets->getRight(), block);
				}
			}
		}
		return false;
	}

	string standard(string s, int l = 6) {
		stringstream str;
		str << s;
		for (int i = l; i > s.length(); i--) {
			str << " ";
		}
		return str.str();
	}

	string standard(int a, int l = 6) {
		stringstream str;
		str << a;
		for (int i = l; i > abs(a) / 10 + 2; i--) {
			str << " ";
		}
		if (a>=0) str << " "; 
		return str.str();
	}

	
	void prepareTableVariables() {
		varUsed.setSize(3,3);
		// First look which cells need to bee expressed through a variable
		for (int x = 0; x < instance.getWidth(); x++) {
			int i = (x < cellX) ? 0 : ((x < mainX+cellX) ? 1 : 2);
			for (int y = 0; y < instance.getHeight(); y++) {
				int j = (y < cellY) ? 0 : ((y < mainY+cellY) ? 1 : 2);
				if (instance.get(x, y) < 0) varUsed.set(i,j,true); 
			}
		}

		if (mainX < cellX) {
			varUsed.set(0,0,true);
			varUsed.set(0,1,true);
			varUsed.set(0,2,true);
		}
		if (mainY < cellY) {
			varUsed.set(0,0,true);
			varUsed.set(1,0,true);
			varUsed.set(2,0,true);
		}
		if (mainX+cellX > instance.getWidth()) {
			varUsed.set(1,0,true);
			varUsed.set(1,1,true);
			varUsed.set(1,2,true);
		}
		if (mainY+cellY > instance.getHeight()) {
			varUsed.set(0,1,true);
			varUsed.set(1,1,true);
			varUsed.set(2,1,true);
		}
		if (instance.getWidth() - mainX - cellX < cellX) {
			varUsed.set(2,0,true);
			varUsed.set(2,1,true);
			varUsed.set(2,2,true);
		}
		if (instance.getHeight() - mainY - cellY < cellY) {
			varUsed.set(0,2,true);
			varUsed.set(1,2,true);
			varUsed.set(2,2,true);
		}
		
		// declare the needed variables
		if (vonNeumann) {
			if (varUsed.get(0, 1)) tableVarDeclaration(0,1);
			if (varUsed.get(1, 0)) tableVarDeclaration(1,0);
			if (varUsed.get(1, 1)) tableVarDeclaration(1,1);
			if (varUsed.get(1, 2)) tableVarDeclaration(1,2);
			if (varUsed.get(2, 1)) tableVarDeclaration(2,1);
		} else {
			for (int x = 0; x < 3; x++)
				for (int y = 0; y < 3; y++)
					if (varUsed.get(x, y)) tableVarDeclaration(x,y);
		}
	}

	void tableVarDeclaration(int i, int j) {
		// prepare cell instance
		Picture<int> p = Picture<int>(-1);             // cell instance
		Picture<bool> stencil = Picture<bool>(false);  // stencil > 0 => Field is used
		p.setSize(cellX,cellY);
		stencil.setSize(cellX,cellY);
		int xt = (i == 0) ? mainX-cellX : ((i == 1) ? mainX : mainX+cellX);
		int yt = (j == 0) ? mainY-cellY : ((j == 1) ? mainY : mainY+cellY);
		int k = 1;
		for (int x = 0; x < cellX; x++) 
			for (int y = 0; y < cellY; y++) {
				if (setLists.get(x,y).get()) p.set(x,y,0);
				if (instance.get(xt+x, yt+y) >= 0) {
					stencil.set(x,y,true);
					k *=setLists.get(x,y)->size();
				}
			}

		// iterate through all cell instances
		vector<vector<int>> variables = vector<vector<int>>(k); 
		bool finished = false;
		while (!finished) {
			int num(0), num2(0);
			int i(1), i2(1);
			for (int x = 0; x < cellX; x++) 
				for (int y = 0; y < cellY; y++) {
					if (setLists.get(x,y).get()) {
						num += p.get(x,y) * i;
						i   *= setLists.get(x,y)->size();
						if (stencil.get(x,y)) {
							num2 += p.get(x,y) * i2;
							i2   *= setLists.get(x,y)->size();
						}
					}
				}
			variables[num2].push_back(num);
			finished = generateCellInstance(p);
		}

		for (int l = 0; l < k; l++) {
			tableStream << "var ";
			if (j == 0)       tableStream << "N";
			else if (j == 2)  tableStream << "S";
			if (i == 0)	      tableStream << "W";
			else if (i == 2)  tableStream << "E";
			if (i==1 && j==1) tableStream << "C";
			tableStream << l;
			tableStream << " = {";
			tableStream << variables[l][0];
			for (int k = 1; k < variables[l].size(); k++) {
				tableStream << ", " << variables[l][k];
			}
			tableStream << "}" << endl;
		}
		tableStream << endl;
	}
	
	bool generateCellInstance(Picture<int> & p) {
		for (int x = 0; x < cellX; x++)
			for (int y = 0; y < cellY; y++) {
				if (p.get(x,y) >= 0) {
					p.set(x,y, p.get(x,y) + 1);
					if (p.get(x,y) >= setLists.get(x,y)->size()) p.set(x,y,0);
					else return false;
				}
			}
		return true;
	}


	void printTableInstance() {
		if (vonNeumann) {
			printTableCell(1,1);
			printTableCell(1,0);
			printTableCell(2,1);
			printTableCell(1,2);
			printTableCell(0,1);
		} else {
			printTableCell(1,1);
			printTableCell(1,0);
			printTableCell(2,0);
			printTableCell(2,1);
			printTableCell(2,2);
			printTableCell(1,2);
			printTableCell(0,2);
			printTableCell(0,1);
			printTableCell(0,0);
		}
	}

	void printTableCell(int i, int j) {
		Picture<int> p = Picture<int>(-1);
		p.setSize(cellX,cellY);
		int xt = (i == 0) ? mainX-cellX : ((i == 1) ? mainX : mainX+cellX);
		int yt = (j == 0) ? mainY-cellY : ((j == 1) ? mainY : mainY+cellY);
		for (int x = 0; x < cellX; x++) 
			for (int y = 0; y < cellY; y++) 
				p.set(x,y,instance.get(xt+x, yt+y));
		
		if (!varUsed.get(i,j)) {
			printTableCell(p);
		} else {
			if (j == 0)       tableStream << "N";
			else if (j == 2)  tableStream << "S";
			if (i == 0)	      tableStream << "W";
			else if (i == 2)  tableStream << "E";
			if (i==1 && j==1) tableStream << "C";
			
			int num(0), k(1);
			for (int x = 0; x < cellX; x++) 
				for (int y = 0; y < cellY; y++) {
					if (p.get(x,y) >= 0) {
						num  += p.get(x,y) * k;
						k *= setLists.get(x,y)->size(); 
					}
				}
			tableStream << num;
		}

		tableStream << ", ";
	}
	
	void printTableCell(Picture<int> & p) {
		int num = 0;
		int k = 1;
		for (int x = 0; x < cellX; x++) 
			for (int y = 0; y < cellY; y++) {
				if (setLists.get(x,y).get()) {
					num  += p.get(x,y) * k;
					k *= setLists.get(x,y)->size();
				}
			}

		tableStream << num;
	}
};

#endif