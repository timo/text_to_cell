#ifndef _TILING_AUTOMATON_H_
#define _TILING_AUTOMATON_H_

#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "TileTree.h"
#include "Picture.h"

class TilingAutomaton {
 public:
	// Standard used in this program recognizes specifically the tables used here
	TilingAutomaton() {
		string standardTiles [] = {//"+-|x", problem: ambiguity with "+-|d"
		"-+x|","|x+-", "x|-+", "###+", "##+#", "#+##", "+###",
		"#+#|", "+#|#", "#|#+", "|#+#", "##+-", "##-+", "+-##", "-+##", "##--", "--##", "#|#|", "|#|#",
		"xxxx", "|x|x", "x|x|", "--xx", "xx--", 
		"aaaa", "a+a|", "a|a|", "a|-+", "a|a+", "aa--", "aa+-", "aa-+", "###a", "##aa", "#a#a", "#a#+", "##a+", "aaa+",
		"bbbb", "+b|b", "|b|b", "|b+-", "|b+b", "bb--", "bb+-", "bb-+", "##b#", "##bb", "b#b#", "b#+#", "##+b", "bb+b",
		"cccc", "c|c+", "c|c|", "-+c|", "c+c|", "--cc", "+-cc", "-+cc", "#c##", "cc##", "#c#c", "#+#c", "c+##", "c+cc",
		"dddd", "|d+d", "|d|d", /*"+-|d",*/ "+d|d", "--dd", "+-dd", "-+dd", "d###", "dd##", "d#d#", "+#d#", "+d##", "+ddd",
		"+-|y", "-+y|", "|y+-", "y|-+", "--yx", "yxxx", "|y|x", "yx--", "y|x|",
		"+-|z", "--zz", "|z|z", "zzzz", "-+zd", "|z+d", "zzdd", "zdzd", "zddd", "-+z|", "|z+-", "zz--", "z|z|", "z|-+", "--zy", "zy--", "|z|y", "z|y|", "zyyy"};
		tiles = new TileTree(standardTiles, 107);
		pi['#']="#";
		pi['+']="+";
		pi['-']="-";
		pi['|']="|";
		pi['x']=" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-/*(){},?.=_<>$%";// hopefully nearly everything
		pi['y']="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-/*(){},?.=_<>$%";
		pi['z']=" ";
		pi['a']=" ";
		pi['b']=" ";
		pi['c']=" ";
		pi['d']=" ";
	}

	TilingAutomaton(string tileStrings[], int tilesLength, char orderedAlphabet[], int alphabetLength, string piInAlphabetsOrder[]) {
		tiles = new TileTree(tileStrings, tilesLength);
		for(int i = 0; i < alphabetLength; i++) pi[orderedAlphabet[i]]=piInAlphabetsOrder[i];
	}

	~TilingAutomaton() {
		delete tiles;
	}

	bool testPicture(Picture<char> * pic) {
		Picture<char> tempPic = Picture<char>(pic->getWidth(), 2, '#');
		
		string tStr;
		// Possibly make own class for scanning strategy
		for(int j = 0; j < pic->getHeight(); j++) {
			for(int i = 0; i < pic->getWidth(); i++) {
				tStr.clear();
				tStr.push_back(tempPic.get(i-1,0));
				tStr.push_back(tempPic.get(i,0));
				tStr.push_back(tempPic.get(i-1,1));
				string ttStr = tiles->getPossibleEndings(tStr,0);
				int sign = -1;
				for (int k = 0; k < ttStr.size(); k++) {
					if (pi[ttStr[k]].find(pic->get(i, j)) < pi[ttStr[k]].size()) sign = k;
				}
				if (sign >= 0) tempPic.set(i,1, ttStr[sign]);
				else return false;
				/*else {
					ofstream outStream;
					outStream.open("outTAuto.txt");
					outStream << "Position: " << i << ","<< j << endl;
					outStream << "Symbol: " << pic->get(i, j) << endl;
					outStream << "Tile: " << tStr << " possibilities: " << ttStr << endl;
					for (int i = 0; i < tempPic.getHeight(); i++) {
						for (int j = 0; j < tempPic.getWidth(); j++) {
							outStream << tempPic.get(j,i);
						}
						outStream << endl;
					}
					outStream.close();
					return false;
				}//*/
			}
			for (int i = 0; i < tempPic.getWidth(); i++) {
				tempPic.set(i,0, tempPic.get(i, 1));
				tempPic.set(i,1,'#');
			}
		}

		return true;
	}
	
private:
	TileTree * tiles;
	// stores the projection pi which is used in the theory of 2D languages
	map<char, string> pi;
};

#endif