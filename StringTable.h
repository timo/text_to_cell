#ifndef _STR_TABLE_H_
#define _STR_TABLE_H_

#include <cstdlib>
#include <map>
#include <string>

using namespace std;

class StringTable {
public:
	
	StringTable() : counter(1){
	}

	int getIdentity(string str) {
		if(table[str] == 0) {
			table[str] = counter;
			revTable[counter] = str;
			counter++;
		}
		return table[str];
	}

	string getString(int ident) {
		return revTable[ident];
	}

private:
	map<string, int> table; // would have to be changed to incorporate namespaces 
							// instead of just int use int and namespace identifier of some kind
							// or maybe even multiple StringTables for different namespaces
	map<int, string> revTable;


	int counter;
};


#endif