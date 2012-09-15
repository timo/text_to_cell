// vim: noet
#ifndef _ZCG_H_
#define _ZCG_H_

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
#include "Variable.h"

#define _ZASIM_CODE_GEN_DEBUG

#ifdef _ZASIM_CODE_GEN_DEBUG
#define toOutStream outStream << "</span><span title=\"" << __LINE__ << "\" style=\"color: hsl(" << ((__LINE__ * 1567396739L) % 360) << ", 100%, 75%);\">"
#else
#define toOutStream outStream
#endif

class ZasimCodeGenerator {
public:
	ZasimCodeGenerator(StringTable & strTable, map<int, counted_ptr<Variable>> & varTable, string name)
	: varTable(varTable), strTable(strTable), posSet(counted_ptr<Set>(new Set())) {
		outStream.open(name + ".zac");
#ifdef _ZASIM_CODE_GEN_DEBUG
		outStream << "<body bgcolor=\"black\"><pre>";
		outStream << "<span>";
#endif
	}

	~ZasimCodeGenerator() {
		outStream.close();
	}

	bool generateCode(CellFile & _program, Picture<counted_ptr<vector<CellStatement>>> &_setLists) {
		setLists = &_setLists;
		program = &_program;
		cellX = program->head.getCell()->getWidth();
		cellY = program->head.getCell()->getHeight();
		posSet.setSize(cellX, cellY);

		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = program->head.getCell();
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (pic->get(i,j)->getType() != EMPTY) {
					posSet.set(i, j, pic->get(i,j)->getSet());
				}
			}

		writeHead();
		writeSymbols();
		writeNeighbourhood();
		python_mode = true;
		writeFunction(*program);
		toOutStream << "," << endl;
		python_mode = false;
		writeFunction(*program);
		// ...
		writeEnd();
		return error.str().empty();
	}

	string getError() {
		return error.str();
	}

private:
	map<int, counted_ptr<Variable>> & varTable;
	StringTable & strTable;
	stringstream error;
	Picture<counted_ptr<vector<CellStatement>>> *setLists;
	CellFile *program;
	//string getNeighbors, giveNeighbors;
	bool python_mode;

	int cellX, cellY;
	Picture<counted_ptr<Set>> posSet;
	ofstream outStream;
	
	void writeHead() {
		toOutStream << "{";
	}

	void writeEnd() {
		toOutStream << "}";
#ifdef _ZASIM_CODE_GEN_DEBUG
		outStream << "</span></pre></body>";
#endif
	}

	string attr(int x, int y) {
		stringstream str;
		str << 'c' << x << 'l'  << y;
		return str.str();
	}

	void writeSetContents(int i, int j) {
		bool putComma(false);
		auto stmt = setLists->get(i, j)->begin();
		auto end = setLists->get(i, j)->end();
		for (;stmt != end; stmt++) {
			if (putComma++) toOutStream << ", ";
			switch (stmt->getType()) {
			case CELL_IDENTIFIER:
				toOutStream << "'" << strTable.getString(stmt->getIdentNumber()) << "'";
				break;
			case CELL_NUMBER:
				toOutStream << stmt->getIdentNumber();
				break;
			default:
				error << "cell " << i << ", " << j << " contains a strange set:" << stmt->print();
			}
		}
	}
	
	/**
	 * Write out the symbol list.
	 * At the moment, there's only the 'symbol' list, but in the future,
	 * each set of different names will have its own list.
	 */
	void writeSymbols() {
		bool putComma(false);
		toOutStream << " 'sets': {" << endl;
		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = program->head.getCell();
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (pic->get(i,j)->getType() != EMPTY) {
                    if (putComma++) toOutStream << "," << endl;
                    toOutStream << "     '";
					toOutStream << attr(i, j);
					toOutStream << "': [";
					writeSetContents(i, j);
					toOutStream << "]";
					posSet.set(i, j, pic->get(i,j)->getSet());
				}
			}
		toOutStream << " }," << endl << endl;
	}

	/**
	 * Write out the "neighbourhood" part. like this:
	 *
	 *  'neighbourhood': [
	 *      {'x': -1, 'y': -1, 'ns': 'A'},
	 *      ...
	 *      ]
	 */
	void writeNeighbourhood() {
		// TODO: Moore Neighbourhood was hardcoded before. make it better.
		toOutStream << " 'neighbourhood': 'MooreNeighbourhood'," << endl;

		return;

		// this is wrong
		toOutStream << " 'neighbourhood': [" << endl;
		bool setComma(false);
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (posSet.get(i,j)->getType() != SET_EMPTY) {
					if (setComma) toOutStream << "," << endl;
					else setComma = true;

					toOutStream << "{'x': " << i << ", 'y': " << j << ", 'ns': ";
					// TODO: give different names to different sets
					if (isIdentInSet(posSet.get(i,j))) {
						toOutStream << "'symbol'";
					} else {
						toOutStream << "'num'";
					}
					toOutStream << "}";
				}
			}
		toOutStream << "]," << endl;
	}

	void writeAttributes() {
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (posSet.get(i,j)->getType() != SET_EMPTY) {
					if (isIdentInSet(posSet.get(i,j))) {
						toOutStream << "  string " << attr(i,j) << ", temp" << attr(i,j) << ";" << endl;
					} else {
						toOutStream << "  int    " << attr(i,j) << ", temp" << attr(i,j) << ";" << endl;
					}
				}
			}
		toOutStream << endl;
	}

	bool isIdentInSet(counted_ptr<Set> set) {
		if (set->getType() == SET_IDENTIFIER) {
			SetIdentifier* s1 = static_cast<SetIdentifier*>(set.get());
			return isIdentInSet(static_cast<VariableSet*>(varTable[s1->getName()].get())->getSet());
		
		} else if (set->getType() == SET_ENUM) {
			return static_cast<SetList*>(set.get())->getIdentifiers().size();
			
		} else if (set->getType() == SET_STATEMENT) {
			SetStatement* s1 = static_cast<SetStatement*>(set.get());
			switch (s1->getOp()) {
			case UNION:					return (isIdentInSet(s1->getLeft()) || isIdentInSet(s1->getRight()));
			case INTERSECTION:			return (isIdentInSet(s1->getLeft()) && isIdentInSet(s1->getRight()));
			case RELATIVE_COMPLEMENT:	return isIdentInSet(s1->getLeft());
				// Could be wrong if all identifiers were taken out by relative complement
										//if (!identInSet(ident, s1->getRight())) return identInSet(ident, s1->getLeft());
										//else return false;
			}
		}
		// should only get here if set is a SET_RANGE these have no identifiers
		return false;
	}

	bool isIdentInSet(int x, int y) {
		int x1 = x % cellX;
		if (x1 < 0) x1 += cellX;
		int y1 = y % cellY;
		if (y1 < 0) y1 += cellY;
		return isIdentInSet(posSet.get(x1,y1));
	}

	void writeFunction(CellFile & file) {
		if (python_mode)
			toOutStream << "'python_code':" << endl;
		else
			toOutStream << "'c_code':" << endl;
		toOutStream << "'";
		if (!file.blocks.empty()) translateBlock(file.blocks[0]);
		for (int i = 1; i < file.blocks.size(); i++) {
			if (python_mode)
				toOutStream << "el";
			else
				toOutStream << " else ";
			translateBlock(file.blocks[i]);
		}
		if (!python_mode)
			toOutStream << endl << "  }";
		toOutStream << "'";
	}


	void translateBlock(Block & block) {
		string AND_S = python_mode ? "and " : "&& ";
		toOutStream << "if (";
		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = block.getLeft();
		bool b = false;
		for (int i = 0; i < pic->getWidth(); i++) {
			for (int j = 0; j < pic->getHeight(); j++) {
				if (pic->get(i,j)->getType() != EMPTY && pic->get(i,j)->getType() != SET_ONLY) {
					// if (no variable initialization) not that important
					if (pic->get(i,j)->getType() != CELL_IDENTIFIER && pic->get(i,j)->getType() != IDENTIFIER_IN_SET) {
						if (b) toOutStream << AND_S << endl << "        ";
						b = true;
						translateCondition(block, i, j);
					} else if (varTable[pic->get(i,j)->getIdentNumber()]->getType() == VAR_CONTENT) {
						VariableContent::Koord k = static_cast<VariableContent *>(varTable[pic->get(i,j)->getIdentNumber()].get())->getKoord(block.getBlockIdent());
						if (k.x != i || k.y != j) {
							if (b) toOutStream << endl << "        " << AND_S;
							b = true;
							translateCondition(block, i, j);
						}
					} else {
						if (b) toOutStream << endl << "        " << AND_S;
						b = true;
						translateCondition(block, i, j);
					}
				}
			}
		}

		for (int i = 0; i < pic->getWidth(); i++) {
			for (int j = 0; j < pic->getHeight(); j++) {
				if (pic->get(i,j)->getType() == IDENTIFIER_IN_SET || 
					pic->get(i,j)->getType() == TERM_IN_SET || 
					pic->get(i,j)->getType() == SET_ONLY) {
					
					if (b) toOutStream << AND_S << endl << "        ";
					b = true;
					
					bool idents = false;
					if (isIdentInSet(i-block.getX(), j-block.getY())) {
						idents = true;
					}
					translateSet(block, pic->get(i,j)->getSet(), idents, i, j);
				}
			}
		}
		
		
		for (int i = 0; i < block.getConstraints().size(); i++) {
			if (b) toOutStream << endl << "        " << AND_S;
			b = true;
			translateTerm(block.getConstraints()[i].getLeft(), block);
			switch(block.getConstraints()[i].getOp()) {
			case OP_EQ_EQ:		toOutStream << " == ";break;
			case OP_LESS:		toOutStream << " < " ;break;
			case OP_LESS_EQ:	toOutStream << " <= ";break;
			case OP_GREATER:	toOutStream << " > " ;break;
			case OP_GREATER_EQ:	toOutStream << " >= ";break;
			case OP_NOT_EQ:		toOutStream << " != ";break;
			}
			translateTerm(block.getConstraints()[i].getRight(), block);
		}

		if (python_mode)
			toOutStream << "):" << endl;
		else
			toOutStream << ") {" << endl;

		translateResult(block);

		if (!python_mode)
			toOutStream << "    }";
	}

	bool translateCondition(Block & block, int x, int y) {
		counted_ptr<CellStatement> c = block.getLeft()->get(x,y);

		getCell(block, x, y);
		toOutStream << " == ";
		if (c->getType() == CELL_IDENTIFIER || c->getType() == IDENTIFIER_IN_SET) {
		
			if (varTable[c->getIdentNumber()]->getType() == SET_CONTENT) {
				toOutStream << '"' << strTable.getString(c->getIdentNumber()) << '"';
			} else if (varTable[c->getIdentNumber()]->getType() == VAR_CONTENT) {
				VariableContent::Koord koord = static_cast<VariableContent *>(varTable[c->getIdentNumber()].get())->getKoord(block.getBlockIdent());
				getCell(block, koord.x, koord.y);
			}

		} else if (c->getType() == CELL_NUMBER) {
			if (isIdentInSet(x-block.getX(), y - block.getY())) toOutStream << '"' << c->getIdentNumber() << '"'; 
			else toOutStream << c->getIdentNumber();
		} else if (c->getType() == CELL_TERM || c->getType() == TERM_IN_SET) {
			translateTerm(c->getTerm(), block);
		}
		return true;
	}

	/*
	void translateSetCondition(Block & block, int x, int y) {
		toOutStream << "inSet" << block.getBlockIdent();
		getCell(block, x, y, false);
		toOutStream << "(";
		getCell(block, x, y);
		toOutStream << ")";
	}*/

	void translateResult(Block & block) {
		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = block.getRight();
		for (int i = 0; i < pic->getWidth(); i++)
			for (int j = 0; j < pic->getHeight(); j++) {
				if (pic->get(i,j)->getType() != EMPTY) {
					toOutStream << "      result_" << attr(i,j) << " = ";
					switch (pic->get(i,j)->getType()) {
					case CELL_NUMBER:
						toOutStream << pic->get(i,j)->getIdentNumber();break;
					case CELL_IDENTIFIER:	
						if (varTable[pic->get(i,j)->getIdentNumber()]->getType() == SET_CONTENT) {
							toOutStream << '"' << strTable.getString(pic->get(i,j)->getIdentNumber()) << '"';
						} else  if (varTable[pic->get(i,j)->getIdentNumber()]->getType() == VAR_CONTENT) {
							VariableContent::Koord koord = static_cast<VariableContent *>(varTable[pic->get(i,j)->getIdentNumber()].get())->getKoord(block.getBlockIdent());
							getCell(block, koord.x, koord.y);
						}
						break;
					case CELL_TERM:
						translateTerm(pic->get(i,j)->getTerm(), block);break;
					default:
						error << "Error: not expected this kind of cell on the right side " << pic->get(i,j)->getType() << " should have been caught by semantics analyser" << endl;
					}
					if (!python_mode)
						toOutStream << ';';
					toOutStream << endl;
				}
			}
	}

	void getCell(Block & block, int x, int y) { //, bool b = true) {
		int x0(x-block.getX()), y0(y-block.getY());
		while(x0 < 0) {
			if (y0 < 0) {
				//toOutStream << ((b) ? "dul->": "dul");
				toOutStream << "dul->";
				y0 += cellY;
			} else if (y0 >= cellY) {
				toOutStream << "ddl->";
				y0 -= cellY;
			} else toOutStream << "left->";
			x0 += cellX;
		}
		while(x0 >= cellX) {
			if (y0 < 0) {
				toOutStream << "dur->";
				y0 += cellY;
			} else if (y0 >= cellY) {
				toOutStream << "ddr->";
				y0 -= cellY;
			} else toOutStream << "right->";
			x0 -= cellX;
		}
		while(y0 < 0) {
			toOutStream << "up->";
			y0 += cellY;
		}
		while(y0 >= cellY) {
			toOutStream << "down->";
			y0 -= cellY;
		}
		toOutStream << attr(x0, y0);
	}

	void writeNextFunction() {
		toOutStream << "  void next() {" << endl;
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (posSet.get(i,j)->getType() != SET_EMPTY) {
					toOutStream << "    " << attr(i,j) << " = result_" << attr(i,j);
					if (!python_mode)
						toOutStream << ";";
					toOutStream << endl;
				}
			}
			toOutStream << "  }" << endl;
	}

	/*
	void writeSetFunctions(CellFile & file) {
		for (int i = 0; i < file.blocks.size(); i++) {
			counted_ptr<Picture<counted_ptr<CellStatement>>> pic = file.blocks[i].getLeft();
			for (int x = 0; x < pic->getWidth(); x++)
				for (int y = 0; y < pic->getHeight(); y++) {
					if (pic->get(x,y)->getType() == SET_ONLY || pic->get(x,y)->getType() == IDENTIFIER_IN_SET) {
						toOutStream << "	bool inSet" << file.blocks[i].getBlockIdent();
						getCell(file.blocks[i], x, y, false);
						toOutStream << "(";
						bool b;
						if (isIdentInSet(x-file.blocks[i].getX(), y-file.blocks[i].getY())) {
							b = true;
							toOutStream << "string str";
						} else {
							b = false;
							toOutStream << "int i";
						}
						toOutStream << ") {" << endl;
						//translateSet(file.blocks[i], pic->get(x,y)->getSet(), b);
						toOutStream << "		return b1;" << endl << "	}" << endl << endl;
					}
				}
		}
	}*/

	void translateSet(Block & block, counted_ptr<Set> set, bool idents, int x, int y) {

		string OR_S = python_mode ? " or " : " || ";
		string AND_S = python_mode ? "and " : "&& ";
		string NOT_S = python_mode ? "not " : "!";

		switch(set->getType()) {
		case SET_IDENTIFIER:	translateSet(block, static_cast<VariableSet*>(varTable[static_cast<SetIdentifier*>(set.get())->getName()].get())->getSet(), idents, x, y); break;
		case SET_ENUM:		{
								SetList * setL = static_cast<SetList *>(set.get());
									
								bool b = false;
								toOutStream << "(";
								if (idents) {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) toOutStream << OR_S;
										else b = true;
										getCell(block,x,y);
										toOutStream << " == " << '"' << setL->getNumbers()[j] << '"';
									}
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) toOutStream << OR_S;
										else b = true;
										getCell(block,x,y);
										toOutStream << " == ";
										if (varTable[setL->getIdentifiers()[j]]->getType() == SET_CONTENT) {
											toOutStream << '"' << strTable.getString(setL->getIdentifiers()[j]) << '"';
										} else if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											getCell(block, koord.x, koord.y);
										}
									}
								} else {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) toOutStream << OR_S;
										else b = true;
										getCell(block,x,y);
										toOutStream << " == " << setL->getNumbers()[j];
									} 
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) toOutStream << OR_S;
										else b = true;
										getCell(block,x,y);
										toOutStream << " == ";
										if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											getCell(block, koord.x, koord.y);
										}
									}
								}
								toOutStream << ")";
								break;
							}
		case SET_RANGE:		{// Can only be used in Number only Sets
								SetRange * setR = static_cast<SetRange *>(set.get());
								if (python_mode)
									toOutStream << "(" << setR->getFirst() << " <= i <= " << setR->getLast() << ")";
								else
									toOutStream << "(i <= " << setR->getLast() << " && i >= " << setR->getFirst() << ")";
								break;
							}
		case SET_STATEMENT:	{
								SetStatement * setS = static_cast<SetStatement *>(set.get());
								toOutStream << "(";
								translateSet(block, setS->getLeft() , idents, x, y);
								switch (setS->getOp()) {
								case UNION:					toOutStream << OR_S; break;
								case INTERSECTION:			toOutStream << endl << "        " << AND_S; break;
								case RELATIVE_COMPLEMENT:	toOutStream << endl << "        " << AND_S << NOT_S; break;
								}
								translateSet(block, setS->getRight(), idents, x, y);
								toOutStream << ")";
								break;
							}
		}
	}
	
	/*void translateSet(Block & block, counted_ptr<Set> set, bool idents, int counter = 1) {

		switch(set->getType()) {
		case SET_IDENTIFIER:	translateSet(block, static_cast<VariableSet*>(varTable[static_cast<SetIdentifier*>(set.get())->getName()].get())->getSet(), idents, counter); break;
		case SET_ENUM:		{
								toOutStream << "		bool b" << counter << " = ";
								SetList * setL = static_cast<SetList *>(set.get());
									
								bool b = false;
								toOutStream << "(";
								if (idents) {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) toOutStream << " ||" << endl << "			   ";
										else b = true;
										toOutStream << "str == " << '"' << setL->getNumbers()[j] << '"';
									}
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) toOutStream << " ||" << endl << "			   ";
										else b = true;
										if (varTable[setL->getIdentifiers()[j]]->getType() == SET_CONTENT) {
											toOutStream << "str == " << '"' << strTable.getString(setL->getIdentifiers()[j]) << '"';
										} else if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											toOutStream << "str == ";
											getCell(block, koord.x, koord.y);
										}
									}
								} else {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) toOutStream << " ||" << endl << "			   ";
										else b = true;
										toOutStream << "i == " << setL->getNumbers()[j];
									} 
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) toOutStream << " ||" << endl << "			   ";
										else b = true;
										if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											toOutStream << "i == ";
											getCell(block, koord.x, koord.y);
										}
									}
								}
								toOutStream << ");" << endl;
								break;
							}
		case SET_RANGE:		{// Can only be used in Number only Sets
								toOutStream << "		bool b" << counter << " = ";
								SetRange * setR = static_cast<SetRange *>(set.get());
								toOutStream << "(i <= " << setR->getLast() << " && i >= " << setR->getFirst() << ");" << endl;
								break;
							}
		case SET_STATEMENT:	{
								SetStatement * setS = static_cast<SetStatement *>(set.get());
								translateSet(block, setS->getLeft() , idents, 2*counter);
								translateSet(block, setS->getRight(), idents, 2*counter +1);
								toOutStream << "		bool b" << counter << " = (b" << 2*counter;
								switch (setS->getOp()) {
								case UNION:					toOutStream << " || b"  << 2*counter + 1 << ");" << endl; break;
								case INTERSECTION:			toOutStream << " && b"  << 2*counter + 1 << ");" << endl; break;
								case RELATIVE_COMPLEMENT:	toOutStream << " && !b" << 2*counter + 1 << ");" << endl; break;
								}
								break;
							}
		}
	}*/

	/*
	void writeConstraintFunctions(CellFile & file) {
		for (int i = 0; i < file.blocks.size(); i++) {
			Block & b = file.blocks[i];
			for (int j = 0; j < b.getConstraints().size(); j++) {
				toOutStream << "	bool constraintB" << b.getBlockIdent() << "C" << j;
				toOutStream << "() {" << endl << "		return ";
				translateTerm(b.getConstraints()[j].getLeft(), b);
				switch(b.getConstraints()[j].getOp()) {
				case OP_EQ_EQ:		toOutStream << " == "; break;
				case OP_LESS:		toOutStream << " < ";  break;
				case OP_LESS_EQ:	toOutStream << " <= "; break;
				case OP_GREATER:	toOutStream << " > ";  break;
				case OP_GREATER_EQ:	toOutStream << " >= "; break;
				case OP_NOT_EQ:		toOutStream << " != "; break;
				}
				translateTerm(b.getConstraints()[j].getRight(), b);
				
				toOutStream << ";" << endl << "	}" << endl << endl;
			}
		}
	}*/

	void translateTerm(counted_ptr<Term> t, Block & b) {
		switch(t->getType()) {
		case T_NUMBER:		toOutStream << static_cast<TermIdentNumber*>(t.get())->getIdentName();
							break;
		case T_IDENTIFIER:	{
								// can only pass semantics test if this Identifier is a locally initialized VariableContent
								VariableContent::Koord k = static_cast<VariableContent*>(
									varTable[static_cast<TermIdentNumber*>(t.get())->getIdentName()].get())
									->getKoord(b.getBlockIdent());
								getCell(b, k.x, k.y);
								break;
							}
		case T_STATEMENT:	{
								TermStatement* ts = static_cast<TermStatement*>(t.get());
								toOutStream << "(";
								translateTerm(ts->getLeft(), b);
								switch(ts->getOp()) {
								case OP_PLUS:	toOutStream << " + "; break;
								case OP_MINUS:	toOutStream << " - "; break;
								case OP_MUL:	toOutStream << " * "; break;
								case OP_DIV:	toOutStream << " / "; break;
								case OP_MOD:	toOutStream << " % "; break;
								}
								translateTerm(ts->getRight(), b);
								
								toOutStream << ")";
								break;
							}
		}
	}


};

#endif
