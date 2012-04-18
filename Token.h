#ifndef _TOKEN_H_
#define _TOKEN_H_
#include <vector>
#include "Picture.h"
#include <string>
#include <sstream>

enum TokenType {
	NUMBER,
	IDENTIFIER,
	PICTURE,
	PICTURE_END,
	CELL,
	MAIN_CELL,

	QMARK,		// not used yet
	EQUALS,
	COMMA,
	
	PLUS,
	MINUS,
	STAR,
	SLASH,
	PERCENT,
	
	TURN,
	MIRROR_X,
	MIRROR_Y,
	NO_POINTER,
	ELSE,

	LINE,
	DOT_DOT_DOT,
	ARROW,
	EQUALS_EQUALS,
	LESS,
	LESS_EQ,
	GREATER,
	GREATER_EQ,
	NOT_EQ,

	IN,
	NIN,

	LBRACE,
	RBRACE,
	LCURLY,
	RCURLY,

	EOFILE,
	ERROR
};

class Token {
public:
	Token(TokenType ptype) {
		type = ptype;
	}

	virtual ~Token() {}

	TokenType getType() {
		return type;
	}

	virtual std::string print() {
		switch(type) {
			case NUMBER: return "<o ohh simple number>";
			case IDENTIFIER: return "<o ohh simple identifier>";
			case PICTURE: return "<o ohh simple picture>";
			case PICTURE_END: return "<PICTURE_END>";
			case CELL: return "<o ohh simple cell>";
			case QMARK: return "<?>";
			case EQUALS: return "<=>";
			case COMMA: return "<,>";
			case PLUS: return "<+>";
			case MINUS: return "<->";
			case STAR: return "<*>";
			case SLASH: return "</>";
			case PERCENT: return "<%>";
			case LESS: return "<<>";
			case LESS_EQ: return "<<=>";
			case GREATER: return "<>>";
			case GREATER_EQ: return "<>=>";
			case LINE: return "<_>";
			case DOT_DOT_DOT: return "<...>";
			case ARROW: return "<=>>";
			case EQUALS_EQUALS: return "<==>";
			case IN: return "<in>";
			case NIN: return "<nin>";
			case LBRACE: return "<(>";
			case RBRACE: return "<)>";
			case LCURLY: return "<{>";
			case RCURLY: return "<}>";
			case MAIN_CELL: return "<main>";
			case TURN: return "<turn>";
			case MIRROR_X: return "<mirror x>";
			case MIRROR_Y: return "<mirror y>";
			case EOFILE: return "<eof>";
			case ERROR: return "<error>";
			default: return "ERROR";
		}
	}

private:
	TokenType type;
};



class NumberToken : public Token {
public:
	NumberToken(int number) : Token(NUMBER), number(number) {}

	virtual ~NumberToken() {}

	int getNumber() {
		return number;
	}
	
	std::string print() {
		std::stringstream out;
		out << "<Number:" << number << ">";
		return out.str();
	}

private:
	int number;
};



class IdentifierToken : public Token {
public:
	IdentifierToken(int identifier) : Token(IDENTIFIER), identifier(identifier) {}

	virtual ~IdentifierToken() {}

	int getIdentity() {
		return identifier;
	}

	std::string print() {
		std::stringstream out;
		out << "<Identifier:" << identifier << ">";
		return  out.str();
	}

private:
	int identifier;
};



class CellToken : public Token {
public:
	CellToken(int x, int y):Token(CELL), x(x), y(y) {}

	virtual ~CellToken() {}

	int getX() {
		return x;
	}

	int getY() {
		return y;
	}

	std::string print() {
		std::stringstream out;
		out << "<Cell:("<< x << "," << y << ")>";
		return out.str();
	}

private:
	int x, y;
};



class PictureToken : public Token {
public:
	PictureToken(int width, int height) : Token(PICTURE), width(width), height(height) {}

	virtual ~PictureToken() {}

	int getWidth() {
		return width;
	}
	
	int getHeight() {
		return height;
	}

	std::string print() {
		std::stringstream out;
		out << "<Picture:("<< width << "," << height << ")>";
		return out.str();
	}

private:
	int width, height;
};

#endif