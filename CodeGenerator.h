#ifndef _CELL_H_
#define _CELL_H_

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

class CodeGenerator {
public:
	CodeGenerator(StringTable & strTable, map<int, counted_ptr<Variable>> & varTable, string name) 
	: varTable(varTable), strTable(strTable), posSet(counted_ptr<Set>(new Set())) {
		outStream.open(name + ".h");
	}

	~CodeGenerator() {
		outStream.close();
	}

	bool generateCode(CellFile & program) {
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

		writeHead();
		writeAttributes(program.noPointer);
		if (!program.noPointer) writeInitializeNeighbors();
		writeInitializeContent();
		writeFunction(program);
		writeNextFunction();
		//writePrivate();
		//writeSetFunctions(program);
		//writeConstraintFunctions(program);
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
	//string getNeighbors, giveNeighbors;

	int cellX, cellY;
	Picture<counted_ptr<Set>> posSet;
	ofstream outStream;


	void writeHead() {
		outStream << "#ifndef _CELL_H_" << endl 
			<< "#define _CELL_H_" << endl << endl << "#include <string>" << endl << endl;
		// Think about includes
		outStream << "class Cell {" << endl 
			<< "public:" << endl;
	}

	void writeEnd() {
		outStream << "};" << endl << endl
			<< "#endif";
	}

	void writeAttributes(bool b) {
		if(!b) {
			outStream << "  Cell * left, * right," << endl
					  << "       * up, * down," << endl
					  << "       * dul, * dur," << endl
					  << "       * ddl, * ddr;"<< endl << endl;
		}

		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (posSet.get(i,j)->getType() != EMPTY) {
					if (isIdentInSet(posSet.get(i,j))) {
						outStream << "  string " << attr(i,j) << ", temp" << attr(i,j) << ";" << endl;
					} else {
						outStream << "  int    " << attr(i,j) << ", temp" << attr(i,j) << ";" << endl;
					}
				}
			}
		outStream << endl;
	}

	void writeInitializeContent() {
		outStream << "  void initializeContent(";
		bool b =  false;
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (posSet.get(i,j)->getType() != EMPTY) {
					if (b) outStream << ", ";
					b = true;
					if (isIdentInSet(posSet.get(i,j))) {
						outStream << "string p" << attr(i,j);
					} else {
						outStream << "int p" << attr(i,j);
					}
				}
			}
		outStream << ") {" << endl;
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (posSet.get(i,j)->getType() != EMPTY) {
					outStream << "    " << attr(i,j) << " = temp" << attr(i,j) << " = p" << attr(i,j) << ";" << endl;
				}
			}
		outStream << "  }" << endl << endl;	
	}

	void writeInitializeNeighbors() {
		outStream << "  void initializeNeighbors(" << endl
				  << "   Cell * p_up, Cell * p_dur," << endl 
		          << "   Cell * p_right, Cell * p_ddr," << endl
				  << "   Cell * p_down, Cell * p_ddl," << endl 
				  << "   Cell * p_left , Cell * p_dul) {" << endl
		          << "    left  = p_left;"  << endl
				  << "    up    = p_up;"    << endl
				  << "    right = p_right;" << endl
				  << "    down  = p_down;"  << endl
				  << "    dul  = p_dul;"  << endl
				  << "    dur  = p_dur;"  << endl
				  << "    ddl  = p_ddl;"  << endl
				  << "    ddr  = p_ddr;"  << endl
		          << "  }" << endl << endl;	
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

	string attr(int x, int y) {
		stringstream str;
		str << 'c' << x << 'l'  << y;
		return str.str();
	}

	void writeFunction(CellFile & file) {
		outStream << "  void computeNext(";
		if (file.noPointer) {
			outStream << endl << "   Cell * up, Cell * dur," << endl 
		          << "   Cell * right, Cell * ddr," << endl
				  << "   Cell * down, Cell * ddl," << endl 
				  << "   Cell * left , Cell * dul";
		}
		outStream << ") {" << endl 
			      << "    ";
		if (!file.blocks.empty()) translateBlock(file.blocks[0]);
		for (int i = 1; i < file.blocks.size(); i++) {
			outStream << " else ";
			translateBlock(file.blocks[i]);
		}
		outStream << endl;
		outStream << "  }" << endl << endl;
	}


	void translateBlock(Block & block) {
		outStream << "if (";
		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = block.getLeft();
		bool b = false;
		for (int i = 0; i < pic->getWidth(); i++) {
			for (int j = 0; j < pic->getHeight(); j++) {
				if (pic->get(i,j)->getType() != EMPTY && pic->get(i,j)->getType() != SET_ONLY) {
					// if (no variable initialization) not that important
					if (pic->get(i,j)->getType() != CELL_IDENTIFIER && pic->get(i,j)->getType() != IDENTIFIER_IN_SET) {
						if (b) outStream << " &&" << endl << "     ";
						b = true;
						translateCondition(block, i, j);
					} else if (varTable[pic->get(i,j)->getIdentNumber()]->getType() == VAR_CONTENT) {
						VariableContent::Koord k = static_cast<VariableContent *>(varTable[pic->get(i,j)->getIdentNumber()].get())->getKoord(block.getBlockIdent());
						if (k.x != i || k.y != j) {
							if (b) outStream << " &&" << endl << "     ";
							b = true;
							translateCondition(block, i, j);
						}
					} else {
						if (b) outStream << " &&" << endl << "     ";
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
					
					if (b) outStream << " &&" << endl << "     ";
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
			if (b) outStream << " &&" << endl << "     ";
			b = true;
			translateTerm(block.getConstraints()[i].getLeft(), block);
			switch(block.getConstraints()[i].getOp()) {
			case OP_EQ_EQ:		outStream << " == ";break;
			case OP_LESS:		outStream << " < " ;break;
			case OP_LESS_EQ:	outStream << " <= ";break;
			case OP_GREATER:	outStream << " > " ;break;
			case OP_GREATER_EQ:	outStream << " >=  ";break;
			case OP_NOT_EQ:		outStream << " != ";break;
			}
			translateTerm(block.getConstraints()[i].getRight(), block);
		}

		outStream << ") {" << endl;

		translateResult(block);

		outStream << "    }";
	}

	bool translateCondition(Block & block, int x, int y) {
		counted_ptr<CellStatement> c = block.getLeft()->get(x,y);
		
		getCell(block, x, y);
		outStream << " == ";
		if (c->getType() == CELL_IDENTIFIER || c->getType() == IDENTIFIER_IN_SET) {
		
			if (varTable[c->getIdentNumber()]->getType() == SET_CONTENT) {
				outStream << '"' << strTable.getString(c->getIdentNumber()) << '"';
			} else if (varTable[c->getIdentNumber()]->getType() == VAR_CONTENT) {
				VariableContent::Koord koord = static_cast<VariableContent *>(varTable[c->getIdentNumber()].get())->getKoord(block.getBlockIdent());
				getCell(block, koord.x, koord.y);
			}

		} else if (c->getType() == CELL_NUMBER) {
			if (isIdentInSet(x-block.getX(), y - block.getY())) outStream << '"' << c->getIdentNumber() << '"'; 
			else outStream << c->getIdentNumber();
		} else if (c->getType() == CELL_TERM || c->getType() == TERM_IN_SET) {
			translateTerm(c->getTerm(), block);
		}
		return true;
	}

	/*
	void translateSetCondition(Block & block, int x, int y) {
		outStream << "inSet" << block.getBlockIdent();
		getCell(block, x, y, false);
		outStream << "(";
		getCell(block, x, y);
		outStream << ")";
	}*/

	void translateResult(Block & block) {
		counted_ptr<Picture<counted_ptr<CellStatement>>> pic = block.getRight();
		for (int i = 0; i < pic->getWidth(); i++)
			for (int j = 0; j < pic->getHeight(); j++) {
				if (pic->get(i,j)->getType() != EMPTY) {
					outStream << "      temp" << attr(i,j) << " = ";
					switch (pic->get(i,j)->getType()) {
					case CELL_NUMBER:
						outStream << pic->get(i,j)->getIdentNumber();break;
					case CELL_IDENTIFIER:	
						if (varTable[pic->get(i,j)->getIdentNumber()]->getType() == SET_CONTENT) {
							outStream << '"' << strTable.getString(pic->get(i,j)->getIdentNumber()) << '"';
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
					outStream << ';' << endl;
				}
			}
	}

	void getCell(Block & block, int x, int y) { //, bool b = true) {
		int x0(x-block.getX()), y0(y-block.getY());
		while(x0 < 0) {
			if (y0 < 0) {
				//outStream << ((b) ? "dul->": "dul");
				outStream << "dul->";
				y0 += cellY;
			} else if (y0 >= cellY) {
				outStream << "ddl->";
				y0 -= cellY;
			} else outStream << "left->";
			x0 += cellX;
		}
		while(x0 >= cellX) {
			if (y0 < 0) {
				outStream << "dur->";
				y0 += cellY;
			} else if (y0 >= cellY) {
				outStream << "ddr->";
				y0 -= cellY;
			} else outStream << "right->";
			x0 -= cellX;
		}		
		while(y0 < 0) {
			outStream << "up->";
			y0 += cellY;
		}
		while(y0 >= cellY) {
			outStream << "down->";
			y0 -= cellY;
		}
		outStream << attr(x0, y0);
	}

	void writeNextFunction() {
		outStream << "  void next() {" << endl;
		for (int i = 0; i < cellX; i++)
			for (int j = 0; j < cellY; j++) {
				if (posSet.get(i,j)->getType() != EMPTY) {
					outStream << "    " << attr(i,j) << " = temp" << attr(i,j) << ";" << endl;
				}
			}
			outStream << "  }" << endl;	
	}

	/*
	void writeSetFunctions(CellFile & file) {
		for (int i = 0; i < file.blocks.size(); i++) {
			counted_ptr<Picture<counted_ptr<CellStatement>>> pic = file.blocks[i].getLeft();
			for (int x = 0; x < pic->getWidth(); x++)
				for (int y = 0; y < pic->getHeight(); y++) {
					if (pic->get(x,y)->getType() == SET_ONLY || pic->get(x,y)->getType() == IDENTIFIER_IN_SET) {
						outStream << "	bool inSet" << file.blocks[i].getBlockIdent();
						getCell(file.blocks[i], x, y, false);
						outStream << "(";
						bool b;
						if (isIdentInSet(x-file.blocks[i].getX(), y-file.blocks[i].getY())) {
							b = true;
							outStream << "string str";
						} else {
							b = false;
							outStream << "int i";
						}
						outStream << ") {" << endl;
						//translateSet(file.blocks[i], pic->get(x,y)->getSet(), b);
						outStream << "		return b1;" << endl << "	}" << endl << endl;
					}
				}
		}
	}*/

	void translateSet(Block & block, counted_ptr<Set> set, bool idents, int x, int y) {

		switch(set->getType()) {
		case SET_IDENTIFIER:	translateSet(block, static_cast<VariableSet*>(varTable[static_cast<SetIdentifier*>(set.get())->getName()].get())->getSet(), idents, x, y); break;
		case SET_ENUM:		{
								SetList * setL = static_cast<SetList *>(set.get());
									
								bool b = false;
								outStream << "(";
								if (idents) {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) outStream << " || ";
										else b = true;
										getCell(block,x,y);
										outStream << " == " << '"' << setL->getNumbers()[j] << '"';
									}
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) outStream << " || ";
										else b = true;
										getCell(block,x,y);
										outStream << " == ";
										if (varTable[setL->getIdentifiers()[j]]->getType() == SET_CONTENT) {
											outStream << '"' << strTable.getString(setL->getIdentifiers()[j]) << '"';
										} else if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											getCell(block, koord.x, koord.y);
										}
									}
								} else {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) outStream << " || ";
										else b = true;
										getCell(block,x,y);
										outStream << " == " << setL->getNumbers()[j];
									} 
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) outStream << " || ";
										else b = true;
										getCell(block,x,y);
										outStream << " == ";
										if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											getCell(block, koord.x, koord.y);
										}
									}
								}
								outStream << ")";
								break;
							}
		case SET_RANGE:		{// Can only be used in Number only Sets
								SetRange * setR = static_cast<SetRange *>(set.get());
								outStream << "(i <= " << setR->getLast() << " && i >= " << setR->getFirst() << ")";
								break;
							}
		case SET_STATEMENT:	{
								SetStatement * setS = static_cast<SetStatement *>(set.get());
								outStream << "(";
								translateSet(block, setS->getLeft() , idents, x, y);
								switch (setS->getOp()) {
								case UNION:					outStream << " || "; break;
								case INTERSECTION:			outStream << " && "; break;
								case RELATIVE_COMPLEMENT:	outStream << " && !"; break;
								}
								translateSet(block, setS->getRight(), idents, x, y);
								outStream << ")";
								break;
							}
		}
	}
	
	/*void translateSet(Block & block, counted_ptr<Set> set, bool idents, int counter = 1) {

		switch(set->getType()) {
		case SET_IDENTIFIER:	translateSet(block, static_cast<VariableSet*>(varTable[static_cast<SetIdentifier*>(set.get())->getName()].get())->getSet(), idents, counter); break;
		case SET_ENUM:		{
								outStream << "		bool b" << counter << " = ";
								SetList * setL = static_cast<SetList *>(set.get());
									
								bool b = false;
								outStream << "(";
								if (idents) {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) outStream << " ||" << endl << "			   ";
										else b = true;
										outStream << "str == " << '"' << setL->getNumbers()[j] << '"';
									}
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) outStream << " ||" << endl << "			   ";
										else b = true;
										if (varTable[setL->getIdentifiers()[j]]->getType() == SET_CONTENT) {
											outStream << "str == " << '"' << strTable.getString(setL->getIdentifiers()[j]) << '"';
										} else if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											outStream << "str == ";
											getCell(block, koord.x, koord.y);
										}
									}
								} else {
									for (int j = 0; j < setL->getNumbers().size(); j++) {
										if (b) outStream << " ||" << endl << "			   ";
										else b = true;
										outStream << "i == " << setL->getNumbers()[j];
									} 
									for (int j = 0; j < setL->getIdentifiers().size(); j++) {
										if (b) outStream << " ||" << endl << "			   ";
										else b = true;
										if (varTable[setL->getIdentifiers()[j]]->getType() == VAR_CONTENT) {
											VariableContent::Koord koord = static_cast<VariableContent *>(varTable[setL->getIdentifiers()[j]].get())->getKoord(block.getBlockIdent());
											outStream << "i == ";
											getCell(block, koord.x, koord.y);
										}
									}
								}
								outStream << ");" << endl;
								break;
							}
		case SET_RANGE:		{// Can only be used in Number only Sets
								outStream << "		bool b" << counter << " = ";
								SetRange * setR = static_cast<SetRange *>(set.get());
								outStream << "(i <= " << setR->getLast() << " && i >= " << setR->getFirst() << ");" << endl;
								break;
							}
		case SET_STATEMENT:	{
								SetStatement * setS = static_cast<SetStatement *>(set.get());
								translateSet(block, setS->getLeft() , idents, 2*counter);
								translateSet(block, setS->getRight(), idents, 2*counter +1);
								outStream << "		bool b" << counter << " = (b" << 2*counter;
								switch (setS->getOp()) {
								case UNION:					outStream << " || b"  << 2*counter + 1 << ");" << endl; break;
								case INTERSECTION:			outStream << " && b"  << 2*counter + 1 << ");" << endl; break;
								case RELATIVE_COMPLEMENT:	outStream << " && !b" << 2*counter + 1 << ");" << endl; break;
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
				outStream << "	bool constraintB" << b.getBlockIdent() << "C" << j;
				outStream << "() {" << endl << "		return ";
				translateTerm(b.getConstraints()[j].getLeft(), b);
				switch(b.getConstraints()[j].getOp()) {
				case OP_EQ_EQ:		outStream << " == "; break;
				case OP_LESS:		outStream << " < ";  break;
				case OP_LESS_EQ:	outStream << " <= "; break;
				case OP_GREATER:	outStream << " > ";  break;
				case OP_GREATER_EQ:	outStream << " >= "; break;
				case OP_NOT_EQ:		outStream << " != "; break;
				}
				translateTerm(b.getConstraints()[j].getRight(), b);
				
				outStream << ";" << endl << "	}" << endl << endl;
			}
		}
	}*/

	void translateTerm(counted_ptr<Term> t, Block & b) {
		switch(t->getType()) {
		case T_NUMBER:		outStream << static_cast<TermIdentNumber*>(t.get())->getIdentName();
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
								outStream << "(";
								translateTerm(ts->getLeft(), b);
								switch(ts->getOp()) {
								case OP_PLUS:	outStream << " + "; break;
								case OP_MINUS:	outStream << " - "; break;
								case OP_MUL:	outStream << " * "; break;
								case OP_DIV:	outStream << " / "; break;
								case OP_MOD:	outStream << " % "; break;
								}
								translateTerm(ts->getRight(), b);
								
								outStream << ")";
								break;
							}
		}
	}


};

#endif
