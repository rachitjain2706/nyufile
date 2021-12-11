.PHONY: all
all:
	@ g++ -std=c++1y -lcrypto nyufile.cpp -o nyufile

clean:
	@ rm nyufile