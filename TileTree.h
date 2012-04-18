#ifndef _TILE_TREE_H_
#define _TILE_TREE_H_

#include <cstdlib>
#include <string>
#include <vector>


class TileTree {
 public:
	TileTree(string tiles[], int tilesLength) {
		name = '?'; // not important should not be used anywhere
		sort(tiles, tiles+tilesLength);
		int firstOfKind = 0;
		char kind = tiles->at(0);
		for (int i = 0; i < tilesLength; i++) {
			if ((tiles+i)->at(0) != kind) {
				children.push_back(TileTree(tiles, tilesLength, firstOfKind, i, 1));
				firstOfKind = i;
				kind = (tiles+i)->at(0);
			}
		}
		children.push_back(TileTree(tiles, tilesLength, firstOfKind, tilesLength, 1));
	}

	TileTree(string tiles[], int tilesLength, int first, int end, int depth) {
		name = (tiles+first)->at(depth-1);
		int firstOfKind = first;
		char kind = (tiles+first)->at(depth);
		for (int i = first; i < end; i++) {
			if ((tiles+i)->at(depth) != kind) {
				if (depth < 3) children.push_back(TileTree(tiles, tilesLength, firstOfKind, i, depth+1));
				else possibilities.push_back(kind);
				firstOfKind = i;
				kind = (tiles+i)->at(depth);
			}
		}
		if (depth < 3) children.push_back(TileTree(tiles, tilesLength, firstOfKind, end, depth+1));
		else possibilities.push_back(kind);
	}

	char getName() {
		return name;
	}

	string getPossibleEndings(string str, int depth) {
		if (depth == 3) return possibilities;
		for (int i = 0; i < children.size(); i++) {
			if (children[i].getName() == str[depth]) return children[i].getPossibleEndings(str, depth + 1);
		}
		return "";
	}
	
private:
	char name;
	string possibilities;
	vector<TileTree> children;
};

#endif