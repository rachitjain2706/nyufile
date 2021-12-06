.PHONY: all
all:
	@ g++ -std=c++1y nyufile.cpp -o nyufile

clean:
	@ rm nyufile