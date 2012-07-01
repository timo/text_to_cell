#ifndef _SEMANTICS_ANALYSER_H_
#define _SEMANTICS_ANALYSER_H_

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

class SemanticsAnalyser {
public:
    SemanticsAnalyser(map<int, counted_ptr<Variable> > & varTable)
        : varTable(varTable), posSet(counted_ptr<Set>(new Set())) { }

    bool analyseProgram(CellFile & program) {
        if (!analyseHead(program.head)) return false;
        for (int i = 0; i < program.blocks.size(); i++) {
            if (!analyseBlock(program.blocks[i])) return false;

            if (program.noPointer &&
                program.blocks[i].getX() > cellX ||
                program.blocks[i].getY() > cellY ||
                program.blocks[i].getLeft()->getWidth() - program.blocks[i].getX() - cellX > cellX ||
                program.blocks[i].getLeft()->getHeight() - program.blocks[i].getY() - cellY > cellY) {
                error << "Error: the described cellular automaton accesses cells not in his neighborhood. This is not possible without pointers. It will be continued with pointers." << endl;
                program.noPointer = false;
            }
        }
        return true;
    }

    string getError() {
        return error.str();
    }

private:
    map<int, counted_ptr<Variable> > & varTable;
    stringstream error;
    int cellX, cellY;

    /**
     * Contains what values can be in sets for each subcell.
     */
    Picture<counted_ptr<Set> > posSet;

    bool analyseHead(Head & head) {
        cellX = head.getCell()->getWidth();
        cellY = head.getCell()->getHeight();

        posSet.setSize(cellX,cellY);

        for (int i = 0; i < head.getExpressions().size(); i++) {
            if (!analyseExpression(head.getExpressions()[i])) return false;
        }

        for (int i = 0; i < cellX; i++)
            for (int j = 0; j < cellY; j++) {
                if (head.getCell()->get(i,j)->getType() != EMPTY) {
                    if (head.getCell()->get(i,j)->getType() != SET_ONLY) {
                        error << "Error: in the header each cell musst be 'in <Set>'" << endl;
                        return false;
                    }
                    if (!analyseSet(head.getCell()->get(i,j)->getSet())) {
                        error << "Error: set used in header is not initialized properly" << endl;
                        return false;
                    }
                    posSet.set(i,j,head.getCell()->get(i,j)->getSet());
                }
            }
        return true;
    }

    bool analyseBlock(Block & block) {
        if (!analyseLeftPic(block)) return false;
        if (!analyseRightPic(block)) return false;

        for (int i = 0; i  < block.getConstraints().size(); i++) {
            if (!analyseConstraint(block.getConstraints()[i], block)) return false;
        }

        return true;
    }


    bool analyseExpression(Expression & expr) {
        if (expr.getLeft()->getType() != SET_IDENTIFIER) {
            error << "Error: invalid expression (in header) you cannot define something that is no identifier" << endl;
            return false;
        }

        varTable[static_cast<SetIdentifier *>(expr.getLeft().get())->getName()]
                = counted_ptr<Variable>(new VariableSet(expr.getRight()));

        if (!analyseSet(expr.getRight())) {
            error << "Error: an error ocurred while analysing the right side of an expression in the head" << endl;
            varTable[static_cast<SetIdentifier *>(expr.getLeft().get())->getName()]
                = counted_ptr<Variable>();
            return false;
        }

        return true;
    }


    // Should only be called for sets in header since it automatically labels every identifier it finds inside {..} a constant set content
    bool analyseSet(counted_ptr<Set> set) {

        if (set->getType() == SET_IDENTIFIER) {
            counted_ptr<Variable> var = varTable[static_cast<SetIdentifier*>(set.get())->getName()];
            if (!var.get() || var->getType() != VAR_SET) {
                error << "Error: expected a set instead found an identifier not connected to a set" << endl;
                return false;
            }

        } else if (set->getType() == SET_STATEMENT) {
            return (analyseSet(static_cast<SetStatement*>(set.get())->getLeft())
                && analyseSet(static_cast<SetStatement*>(set.get())->getRight()));

        } else if (set->getType() == SET_ENUM) {

            vector<int> & vec = static_cast<SetList*>(set.get())->getIdentifiers();

            for (int i = 0; i < vec.size(); i++) {
                if (! (varTable[vec[i]].get())) varTable[vec[i]] = counted_ptr<Variable>(new Variable(SET_CONTENT));
                else if (varTable[vec[i]]->getType() == VAR_SET) {
                    error << "Error: there is a set inside another set this is not allowed" << endl;
                    return false;
                }
            }
        }

        // Think about what to do with variables in SET_ENUMs or SET_RANGES
        return true;
    }


    bool analyseLeftPic(Block & block) {
        counted_ptr<Picture<counted_ptr<CellStatement>>> pic = block.getLeft();
        for (int i = 0; i < pic->getWidth() ; i++) {
            int x = (i - block.getX()) % cellX;
            if (x < 0) x += cellX;

            for (int j = 0; j < pic->getHeight(); j++) {
                int y = (j - block.getY()) % cellY;
                if (y < 0) y += cellY;

                if (pic->get(i,j)->getType() == CELL_NUMBER) {
                    // is this number possible in this cell
                    if (!numInSet(pic->get(i,j)->getIdentNumber(), posSet.get(x,y))) {
                        error << "Error: a number on the left side of a block does not fit into its cell" << endl;
                        return false;
                    }

                } else if ((pic->get(i,j)->getType() == CELL_IDENTIFIER)
                        || (pic->get(i,j)->getType() == IDENTIFIER_IN_SET)) {

                    auto identNum = pic->get(i,j)->getIdentNumber();
                    if (!(varTable[identNum].get())) {
                        // never before seen identifier musst be local variable
                        varTable[identNum] = counted_ptr<Variable>(new VariableContent());
                    }

                    if ((varTable[identNum]->getType() == VAR_CONTENT)) {
                        auto varContent = static_cast<VariableContent*>(varTable[pic->get(i,j)->getIdentNumber()].get());
                        if (!(varContent->defInBlock(block.getBlockIdent()))) {
                            // variable was previously undefined in this block has been seen in another block
                            VariableContent::Koord koord;
                            koord.x = i; koord.y = j;
                            varContent->setKoord(block.getBlockIdent(), koord);
                        }

                    } else if ((varTable[identNum]->getType() == SET_CONTENT)) {
                        // the identifier is part of a standard set is this identifier possible in this cell
                        if (!identInSet(identNum, posSet.get(x,y))) {
                            error << "Error: an identifier on the left side of a block does not fit into its cell" << endl;
                            return false;
                        }
                    } else {
                        error << "Error: unexpected identifier in the left picture of a block" << endl;
                        error << "     (probably an identifier connected to a set witout 'in' before it)" << endl;
                        return false;
                    }
                }
            }
        }

        for (int i = 0; i < pic->getWidth() ; i++) {
            int x = (i - block.getX()) % cellX;
            if (x < 0) x += cellX;
            for (int j = 0; j < pic->getHeight(); j++) {
                int y = (j - block.getY()) % cellY;
                if (y < 0) y += cellY;
                if (pic->get(i,j)->getType() == CELL_TERM ||
                    pic->get(i,j)->getType() == TERM_IN_SET) {
                    if (!analyseTerm(pic->get(i,j)->getTerm(), block)) {
                        error << "Error: a term in the left picture of a block is not defined completely" << endl;
                        return false;
                    }
                    if (isIdentInSet(posSet.get(x,y))) {
                        error << "Error: there is a term inside a cell that may contain strings (this is not allowde)" << endl;
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool analyseRightPic(Block & block) {
        counted_ptr<Picture<counted_ptr<CellStatement>>> pic = block.getRight();
        if ((pic->getWidth() != cellX) || (pic->getHeight() != cellY)) {
            error << "Error: right side has not the right dimensions" << endl;
            return false;
        }

        for (int i = 0; i < pic->getWidth() ; i++)
            for (int j = 0; j < pic->getHeight(); j++) {
                if (pic->get(i,j)->getType() == CELL_NUMBER) {
                    // is this number possible in this cell
                    if (!numInSet(pic->get(i,j)->getIdentNumber(), posSet.get(i,j))) {
                        error << "Error: a number on the right side of a block does not fit into its cell" << endl;
                        return false;
                    }

                } else if (pic->get(i,j)->getType() == CELL_TERM) {
                    if (!analyseTerm(pic->get(i,j)->getTerm(), block)) {
                        error << "Error: a term in the right picture of a block is not defined completely" << endl;
                        return false;
                    }
                    if (isIdentInSet(posSet.get(i,j))) {
                        error << "Error: there is a term inside a cell that may contain strings (this is not allowed)" << endl;
                        return false;
                    }

                } else if (pic->get(i,j)->getType() == CELL_IDENTIFIER) {

                    if (!(varTable[pic->get(i,j)->getIdentNumber()].get())) {
                        error << "Error: uninitialized variable on the right side of a block" << endl;
                        return false;

                    } else if (varTable[pic->get(i,j)->getIdentNumber()]->getType() == SET_CONTENT) {
                        // is this identifier possible in this cell
                        if (!identInSet(pic->get(i,j)->getIdentNumber(), posSet.get(i,j))) {
                            error << "Error: an identifier on the right side of a block does not fit into its cell" << endl;
                            return false;
                        }

                    } else if (varTable[pic->get(i,j)->getIdentNumber()]->getType() == VAR_CONTENT) {
                        // is this VAR_IDENTIFIER initialized here?
                        VariableContent* varC = static_cast<VariableContent*>(varTable[pic->get(i,j)->getIdentNumber()].get());
                        if (!varC->defInBlock(block.getBlockIdent())) {
                            error << "Error: uninitialized variable on the right side of a block" << endl;
                            return false;
                        }
                        VariableContent::Koord k = varC->getKoord(block.getBlockIdent());
                        k.x = (k.x - block.getX() < 0) ? ((k.x - block.getX()) % cellX) + cellX : (k.x - block.getX()) % cellX;
                        k.y = (k.y - block.getY() < 0) ? ((k.y - block.getY()) % cellY) + cellY : (k.y - block.getY()) % cellY;
                        if (isIdentInSet(posSet.get(k.x, k.y)) != isIdentInSet(posSet.get(i,j))) {
                            error << "Error: a variable in the right picture has the wrong type" << endl;
                            return false;
                        }

                    } else {
                        error << "Error: unexpected identifier in the right picture of a block" << endl << "     (probably an identifier connected to a set)" << endl;
                        return false;
                    }
                } else if ((pic->get(i,j)->getType() == IDENTIFIER_IN_SET)
                    || (pic->get(i,j)->getType() == SET_ONLY)
                    || (pic->get(i,j)->getType() == TERM_IN_SET)) {
                        error << "Error: Why specify a set on right side of a block?" << endl;
                        return false;
                }
            }

        return true;
    }


    bool analyseConstraint(Constraint & cons, Block & block) {
        if (!analyseTerm(cons.getLeft(), block)) return false;
        if (!analyseTerm(cons.getRight(), block)) return false;
        return true;
    }

    bool analyseTerm(counted_ptr<Term> t, Block & block) {
        if (!t.get()) {
            error << "Error: an empty term" << endl;
            return false;
        }
        switch(t->getType()) {
        case T_NUMBER:		return true;
        case T_IDENTIFIER:	{
                                TermIdentNumber* ti = static_cast<TermIdentNumber*>(t.get());
                                if (!varTable[ti->getIdentName()].get() ||
                                    varTable[ti->getIdentName()]->getType() != VAR_CONTENT) {
                                    error << "Error: identifiers in terms musst be variables" << endl;
                                    return false;
                                }
                                VariableContent* vc = static_cast<VariableContent*>(varTable[ti->getIdentName()].get());
                                if (!vc->defInBlock(block.getBlockIdent())) {
                                    error << "Error: a variable in a term is not defined in this block" << endl;
                                    return false;
                                }
                                VariableContent::Koord k = vc->getKoord(block.getBlockIdent());
                                k.x = (k.x - block.getX()) % cellX;
                                k.y = (k.y - block.getY()) % cellY;
                                k.x = (k.x < 0) ? k.x + cellX : k.x;
                                k.y = (k.y < 0) ? k.y + cellY : k.y;
                                if (isIdentInSet(posSet.get(k.x,k.y))) {
                                    error << "Error: a variable in a term has the wrong type" << endl;
                                    return false;
                                }
                                return true;
                            }
        case T_STATEMENT:	TermStatement* ts = static_cast<TermStatement*>(t.get());
                            return analyseTerm(ts->getLeft(), block) && analyseTerm(ts->getRight(), block);
        }
    }

    // does not work if VAR_CONTENT are in a Set
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

    // does not work if VAR_SET_CONTENTs are in a Set should only be tested to check if SET_CONENT possible in a cell
    // set has to be initialized
    bool identInSet(int ident, counted_ptr<Set> set) {
        if (set->getType() == SET_IDENTIFIER) {
            SetIdentifier* s1 = static_cast<SetIdentifier*>(set.get());
            return identInSet(ident, static_cast<VariableSet*>(varTable[s1->getName()].get())->getSet());

        } else if (set->getType() == SET_ENUM) {
            vector<int> identVec = static_cast<SetList*>(set.get())->getIdentifiers();
            for (int i = 0; i < identVec.size(); i++) {
                if (ident == identVec[i]) return true;
            }

        } else if (set->getType() == SET_STATEMENT) {
            SetStatement* s1 = static_cast<SetStatement*>(set.get());
            switch (s1->getOp()) {
            case UNION: return (identInSet(ident, s1->getLeft()) || identInSet(ident, s1->getRight()));
            case INTERSECTION: return (identInSet(ident, s1->getLeft()) && identInSet(ident, s1->getRight()));
            case RELATIVE_COMPLEMENT: if (!identInSet(ident, s1->getRight())) return identInSet(ident, s1->getLeft());
                                      else return false;
            }
        }
        return false;
    }

    bool numInSet(int num, counted_ptr<Set> set) {
        if (set->getType() == SET_IDENTIFIER) {
            SetIdentifier* s1 = static_cast<SetIdentifier*>(set.get());
            return numInSet(num, static_cast<VariableSet*>(varTable[s1->getName()].get())->getSet());

        } else if (set->getType() == SET_ENUM) {
            vector<int> numVec = static_cast<SetList*>(set.get())->getNumbers();
            for (int i = 0; i < numVec.size(); i++) {
                if (num == numVec[i]) return true;
            }

        } else if (set->getType() == SET_RANGE) {
            int n1 = static_cast<SetRange*>(set.get())->getFirst();
            int n2 = static_cast<SetRange*>(set.get())->getLast();
            return (num >= n1 && num <= n2);

        } else if (set->getType() == SET_STATEMENT) {
            SetStatement* s1 = static_cast<SetStatement*>(set.get());
            switch (s1->getOp()) {
            case UNION:					return (numInSet(num, s1->getLeft()) || numInSet(num, s1->getRight()));
            case INTERSECTION:			return (numInSet(num, s1->getLeft()) && numInSet(num, s1->getRight()));
            case RELATIVE_COMPLEMENT:	if (!numInSet(num, s1->getRight())) return numInSet(num, s1->getLeft());
                                        else return false;
            }
        }
        return false;
    }
};

#endif
