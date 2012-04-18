


#ifdef WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <iostream>
#include <string>
#include <vector>
#include "counted_ptr.h"
#include "Lexer.h"
#include "Picture.h"
#include "TilingAutomaton.h"
#include "TileTree.h"
#include "Token.h"
#include "Set.h"
#include "Term.h"
#include "StringTable.h"
#include "Parser.h"
#include "BasicData.h"
#include "SemanticsAnalyser.h"
#include "CodeGenerator.h"
#include "FunctionAnalyser.h"
/*#include <stdlib.h>
#include <stdio.h>
#include <string.h>
*/

using namespace std;

int main(int argc, char** argv) {
    string name = "example.txt";
    if (argc > 1) name = argv[1];
    string name0 = name.substr(0,name.find_first_of('.'));

    StringTable strTable;
    map<int, counted_ptr<Variable> > varTable;

    Lexer lexer(name, strTable);
    Parser parser(lexer, strTable);
    SemanticsAnalyser analyser(varTable);
    CodeGenerator cgen(strTable, varTable, name0);
    FunctionAnalyser fana(strTable, varTable, name0);

    CellFile file;
    bool b(false),c(false),d(false),e(false);
    b = parser.parseFile(file);
    if (b) c = analyser.analyseProgram(file);
    if (c) d = fana.analyseFunction(file);
    if (d) e = cgen.generateCode(file);


    ofstream outStream;
    outStream.open(name0 + "_log.txt");
    /*
    outStream << "remaining tokens:    ";
    counted_ptr<Token> token = lexer.next();
    while (token->getType() != EOFILE) {
        outStream << token->print();
        token = lexer.next();
    }
    outStream << token->print() << endl << endl;
    */
    outStream << "parsing file " << name << endl << endl;

    outStream << "parsing:             " << (b? "successful": "failure") << endl;
    outStream << "semantics analysis:  " << (c? "successful": "failure") << endl;
    outStream << "function analysis:   " << (d? "successful": "failure") << endl;
    outStream << "code generator:      " << (e? "successful": "failure") << endl << endl;

    outStream << "parse error:         " << parser.getError()   << endl;
    outStream << "semantics error:     " << analyser.getError() << endl;
    outStream << "function error:      " << fana.getError()     << endl;
    outStream << "generator error:     " << cgen.getError()     << endl << endl;

    /*
    counted_ptr<Picture<counted_ptr<CellStatement>>> pic = file.head.getCell();
    for (int i = 0; i < pic->getHeight(); i++) {
        for (int j = 0; j < pic->getWidth(); j++) {
            outStream << pic->get(j,i)->print();
        }
        outStream << endl;
    }

    for (int k = 0; k < file.blocks.size(); k++) {
        outStream << endl;
        pic = file.blocks[k].getLeft();
        for (int i = 0; i < pic->getHeight(); i++) {
            for (int j = 0; j < pic->getWidth(); j++) {
                outStream << pic->get(j,i)->print();
            }
            outStream << endl;
        }
        outStream << file.blocks[k].getX() << " " <<file.blocks[k].getY() << endl;
        outStream << endl;
        pic = file.blocks[k].getRight();
        for (int i = 0; i < pic->getHeight(); i++) {
            for (int j = 0; j < pic->getWidth(); j++) {
                outStream << pic->get(j,i)->print();
            }
            outStream << endl;
        }
        outStream << endl;
    }

    for (map<int, counted_ptr<Variable>>::iterator it = varTable.begin(); it != varTable.end(); it++) {
        outStream << it->first << ":  ";
        if (it->second.get()) outStream << it->second->print() << endl;
    }
    outStream.close();
    */
    return 0;
}
