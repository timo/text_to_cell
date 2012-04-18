#ifndef _PICTURE_H_
#define _PICTURE_H_

template <class T>
class Picture {
 public:

	// defaultT = NULL should be used for complex data structures
	Picture(T defaultT) : defaultT(defaultT) {
		width = height = 0;
		contents = new T [1];
		contents[0] = defaultT;
	}
  
	Picture(int width, int height, T defaultT) : width(width), height(height), defaultT(defaultT) {
		contents = new T [width * height];
		for (int i = 0; i < width*height ; i++) contents[i]= defaultT;
	}

	~Picture() {
		delete [] contents; //why is this throwing E´s
	}
  
	int getWidth() const {
		return width;
	}
  
	int getHeight() const {
		return height;
	}
  
	void setSize(int width, int height) {
		delete [] contents;
		this->width  = width;
		this->height = height;
		contents = new T [width * height];
		for (int i = 0; i < width*height ; i++) contents[i] = defaultT;
	}
  
	const T get(int i, int j) {
		if ((0 <= i) && (i < width) && (0 <= j) && (j < height)) 
			return contents[i + j * width];
		else return defaultT;
	}
  
	void set(int i, int j, T content) {
		if ((0 <= i) && (i < width) && (0 <= j) && (j < height)) 
			contents[i + j * width] = content;
	}

private:
	T * contents;
	T defaultT;
	int width, height;
};

#endif