#ifndef _LEXER_H_
#define _LEXER_H_

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include "Token.h"
#include "StringTable.h"
#include "TilingAutomaton.h"
#include <queue>

using namespace std;

class Lexer {
public:
	Lexer(string fileName, StringTable & pStrTbl) : strTbl(pStrTbl), posy(-1) {
		readToVector(fileName);
	}

	counted_ptr<Token> next() {
		while (buffer.empty()) {
			if (++posy < text.size()) parseString(text[posy]);
			else return counted_ptr<Token>(new Token(EOFILE));
		}
		counted_ptr<Token>  result = buffer.front();
		buffer.pop();
		return result;
	}

	void parseString(string line, bool inPic = false) {
		int x = 0;
		while (x < line.size()) {
			switch(line[x]) {
				case ' ': x++; break;
				case '-': x++; buffer.push(counted_ptr<Token>(new Token(MINUS))); break;
				case '*': x++; buffer.push(counted_ptr<Token>(new Token(STAR)));break;
				case '%': x++; buffer.push(counted_ptr<Token>(new Token(PERCENT)));break;
				case '(': x++; buffer.push(counted_ptr<Token>(new Token(LBRACE)));break;
				case ')': x++; buffer.push(counted_ptr<Token>(new Token(RBRACE)));break;
				case '{': x++; buffer.push(counted_ptr<Token>(new Token(LCURLY)));break;
				case '}': x++; buffer.push(counted_ptr<Token>(new Token(RCURLY)));break;
				case ',': x++; buffer.push(counted_ptr<Token>(new Token(COMMA)));break;
				case '?': x++; buffer.push(counted_ptr<Token>(new Token(QMARK)));break;
				case '$': x++; buffer.push(counted_ptr<Token>(new Token(MAIN_CELL)));break;
				case '.': 
					while ((x < line.size()) && ((line[x] == '.') || (line[x] == ' '))) x++; 
					buffer.push(counted_ptr<Token>(new Token(DOT_DOT_DOT)));
					break;
				case '/': 
					if (++x < line.size()) {
						if (line[x] == '/') {
							x = line.size();
							// is meant for comments
							break;
						}
					}
					buffer.push(counted_ptr<Token>(new Token(SLASH)));break;
				case '=':	
					if (++x < line.size()) {
						if (line[x] == '=') {
							x++;
							buffer.push(counted_ptr<Token>(new Token(EQUALS_EQUALS)));
						} else if (line[x] == '>') {
							x++;
							buffer.push(counted_ptr<Token>(new Token(ARROW)));
						} else buffer.push(counted_ptr<Token>(new Token(EQUALS)));
					} else buffer.push(counted_ptr<Token>(new Token(EQUALS)));
					break;
				case '<':	
					if (++x < line.size() && line[x] == '=') {
							x++;
							buffer.push(counted_ptr<Token>(new Token(LESS_EQ)));
					} else buffer.push(counted_ptr<Token>(new Token(LESS)));
					break;
				case '>':	
					if (++x < line.size() && line[x] == '=') {
							x++;
							buffer.push(counted_ptr<Token>(new Token(GREATER_EQ)));
					} else buffer.push(counted_ptr<Token>(new Token(GREATER)));
					break;
				case '!':	
					if (++x < line.size() && line[x] == '=') {
							x++;
							buffer.push(counted_ptr<Token>(new Token(NOT_EQ)));
					} else buffer.push(counted_ptr<Token>(new Token(ERROR)));
					break;
				case '+':
					if ((++x < line.size()) && (line[x] == '-') && !inPic) {
						parsePicture(x -1, posy);
						parseString(text[posy]);
						return;
					} else buffer.push(counted_ptr<Token>(new Token(PLUS)));
					break;
				case '_': 
					while ((x < line.size()) && (line[x] == '_')) x++; 
					buffer.push(counted_ptr<Token>(new Token(LINE)));
					break;
				default:    
					if (isDigit(line[x])) {
						x = parseNumber(line, x);
						break;
					} else if (isChar(line[x])) {
						x = parseIdentifier(line, x);
						break;
					}
					//should not be reached
					buffer.push(counted_ptr<Token>(new Token(ERROR)));
					return;
			}
		}
	}

private:
	vector<string> text;
	int posy;
	StringTable & strTbl;
	queue<counted_ptr<Token>> buffer;
	TilingAutomaton tAuto;

	void readToVector(const string & fileName) {

		ifstream fileStream(fileName, ios::in);
		if (!fileStream) return;
		
		string line;

		while(getline(fileStream,line)) {
			text.push_back(line);
		}

		fileStream.close();
	}

	inline bool isDigit(char c) {
		return ((c >= '0') && (c <= '9'));
	}

	inline int digitToInt(char c) {
		return c - '0';
	}

	inline bool isChar(char c) {
		return (((c <= 'z') && (c >= 'a')) || ((c <= 'Z') && (c >= 'A')));
	}

	//void parsePicture(int x, int y) {
	//	posy = picRecognizer.parsePicture(&text, x, y);
	//	//posy += pic.GetHeight() - 1;
	//}

	int parseNumber(const string & line, int x) {
		int posx = x;
		int result = digitToInt(line[posx]);
		while ((++posx < line.size()) && isDigit(line[posx])) {
			result *= 10;
			result += digitToInt(line[posx]);
		}
		buffer.push(counted_ptr<Token>(new NumberToken(result)));
		return posx;
	}

	int parseIdentifier(const string & line, int x) {
		int posx = x;
		string result = "";
		result.push_back(line[posx]);
		while ((++posx < line.size()) && (isChar(line[posx]) || isDigit(line[posx]))) result.push_back(line[posx]);
		
		if (result == "in") {
			buffer.push(counted_ptr<Token>(new Token(IN)));
			return posx;
		} else if (result == "turn") {
			buffer.push(counted_ptr<Token>(new Token(TURN)));
			return posx;
		} else if (result == "else") {
			buffer.push(counted_ptr<Token>(new Token(ELSE)));
			return posx;
		} else if (result == "mirrorX") {
			buffer.push(counted_ptr<Token>(new Token(MIRROR_X)));
			return posx;
		} else if (result == "mirrorY") {
			buffer.push(counted_ptr<Token>(new Token(MIRROR_Y)));
			return posx;
		} else if (result == "nopointer") {
			buffer.push(counted_ptr<Token>(new Token(NO_POINTER)));
			return posx;
		}
		
		int ident = strTbl.getIdentity(result);
		buffer.push(counted_ptr<Token>(new IdentifierToken(ident)));
		return posx;
	}
	
	// has to be called with the highest leftest +
	void parsePicture(int x, int y) {
		vector<int> xvec, yvec;

		int i= 0;
		//get initial width
		while((x+i) < text.at(y).size()) {
			if (text.at(y)[x+i] == '+') xvec.push_back(x+i);
			else if (text.at(y)[x+i] != '-') break;
			i++;
		}

		i=0;
		//get initial height
		while(((y+i) < text.size()) && (x < text[y+i].size())) {
			if (text.at(y+i)[x] == '+') yvec.push_back(y+i);
			else if (text.at(y+i)[x] != '|') break;
			i++;
		}
		
		
		// expand (if formed like a cross)
		// expand left
		for (int k = 0; k < yvec.size(); k++) {
			i = 1;
			while ((xvec.front()-i) >= 0) {
				if (text.at(yvec[k])[xvec.front()-i] == '+') {
					xvec.insert(xvec.begin(), xvec.front()-i);
					i=1;
				} else if (text.at(yvec[k])[xvec.front()-i] != '-') break;
				i++;
			}
		}
		//expand right
		for (int k = 0; k < yvec.size(); k++) {
			i = 1;
			while ((xvec.back()+i) < text.at(yvec[k]).size()) {
				if (text.at(yvec[k])[xvec.back()+i] == '+') {
					xvec.push_back(xvec.back()+i);
					i=1;
				} else if (text.at(yvec[k])[xvec.back()+i] != '-') break;
				i++;
			}
		}
		//expand bottom
		for (int k = 0; k < xvec.size(); k++) {
			i = 1;
			while (((yvec.back() + i) < text.size()) && (xvec[k] < text.at(yvec.back()+i).size())) {
				if (text.at(yvec.back()+i)[xvec[k]] == '+') {
					yvec.push_back(yvec.back()+i);
					i=1;
				} else if (text.at(yvec.back()+i)[xvec[k]] != '|') break;
				i++;
			}
		}
		
		//test yvec there cannot be two + directly between each other this could mean a plus from a constraint got mixed up in a picture
		for (int i = 1; i < yvec.size(); i++) {
			if (yvec[i-1] == yvec[i] -1) yvec.erase(yvec.begin()+i, yvec.end());
		}

		//copy Picture from vector to pic
		Picture<char> pic = Picture<char>('#');
		pic.setSize(xvec.back() - xvec.front() + 1, yvec.back() - yvec.front() + 1);
		for(int l = yvec.front(); l <= yvec.back(); l++) {
			for(int k = xvec.front(); k <= xvec.back(); k++) {
				if (k < text.at(l).size()) pic.set(k - xvec.front(),l - yvec.front(),text.at(l).at(k));
				else pic.set(k - xvec.front(),l - yvec.front(),' ');
			}
		}


		if (tAuto.testPicture(&pic)) {
			//parse Picture
			buffer.push(counted_ptr<Token>(new PictureToken(xvec.size() -1, yvec.size() -1)));
			int xt(xvec.front()), yt(yvec.front());
			for (int i = 0; i < xvec.size(); i++) xvec[i] -= xt;
			for (int i = 0; i < yvec.size(); i++) yvec[i] -= yt;
			for (int i = 0; i < (xvec.size() -1); i++) {
				for (int j = 0; j < (yvec.size() -1); j++) {
					//parse cell (i,j  ) (i+1,j  )
					//           (i,j+1) (i+1,j+1)

					
					// if cell is interior
					if (pic.get(xvec[i]  , yvec[j]  ) == '+' &&
						pic.get(xvec[i+1], yvec[j]  ) == '+' &&
						pic.get(xvec[i]  , yvec[j+1]) == '+' &&
						pic.get(xvec[i+1], yvec[j+1]) == '+') {
						
						buffer.push(counted_ptr<Token>(new CellToken(i,j)));
						
						string cell = "";
						for (int k = yvec[j]+1; k < yvec[j+1]; k++) {
							for (int l = xvec[i]+1; l < xvec[i+1]; l++) {
								cell.push_back(pic.get(l,k));
							}
							cell.push_back(' ');
						}
						parseString(cell, true);
					}
				}
			}
			buffer.push(counted_ptr<Token>(new Token(PICTURE_END)));

			for (int j = yt; j <= yvec.back() + yt; j++) {
				//text[j].erase(xt, xvec.back() + 1);
				for (int i = xt; i <= xvec.back() + xt; i++) {
					text[j][i] = ' ';
				}
			}

		} else {
			buffer.push(counted_ptr<Token>(new Token(ERROR)));
			for (int j = yvec.front(); j <= yvec.back(); j++)
				//text[j].erase(xvec.front(), xvec.back() - xvec.front() + 1);
				for (int i = xvec.front(); i <= xvec.back(); i++) {
					text[j][i] = ' ';
				}
		}
	}

};

#endif