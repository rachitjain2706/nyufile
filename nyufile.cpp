#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "dataStructers.h"
#include <stdlib.h>

using namespace std;

bool sha1 = false;
bool readFileSystemInformation = true; // TODO: Init to true for M1 due to getopt
bool readRootDirectoryInformation = true; // TODO: Init to true for M1 due to getopt
char* diskName;

void fileSystemInformation(struct BootEntry bootEntry) {
    int disk = open(diskName, O_RDONLY);
    pread(disk, &bootEntry, sizeof(bootEntry), 0);

    cout << "Number of FATs = " <<  (int)bootEntry.BPB_NumFATs << endl;
    cout << "Number of bytes per sector = " << bootEntry.BPB_BytsPerSec << endl;
    cout << "Number of sectors per cluster = " << (int) bootEntry.BPB_SecPerClus << endl;
    cout << "Number of reserved sectors = " << (int) bootEntry.BPB_RsvdSecCnt << endl;
}

void rootDirectoryInformation() {

}

void printUsage() {
    cout << "Usage: ./nyufile disk <options>" << endl;
    cout << "-i\tPrint the file system information." << endl;
    cout << "-l\tList the root directory." << endl;
    cout << "-r filename [-s sha1]\tRecover a contiguous file." << endl;
    cout << "-R filename -s sha1\tRecover a possibly non-contiguous file." << endl;
}

int parseInput(int argc, char *argv[]) {
    int c;
    string fileName;
    opterr = 0;
    if (argc <= 1) {
        printUsage();
        return 0;
    }
    diskName = argv[1];
    while ((c = getopt(argc, argv, "ilr:R:s:")) != -1) {
        switch (c) {
            case 'i':
                cout << "DEBUG: Here 1";
                readFileSystemInformation = true;
                break;
            case 'l':
                cout << "DEBUG: Here 2";
                readRootDirectoryInformation = true;
                break;
            case 'r':
                fileName = optarg;
                cout << "DEBUG: Here 3" << fileName;
                break;
            case 'R':
                fileName = optarg;
                cout << "DEBUG: Here 4" << fileName;
                break;
            case 's':
                sha1 = true;
                cout << "DEBUG: Here 5" << sha1;
                break;
            default:
                cout << "DEBUG: Here 6" << sha1;
                printUsage();
                return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    parseInput(argc, argv);
    cout << diskName << endl;
    struct BootEntry bootEntry;
    if (readFileSystemInformation) {
        fileSystemInformation(bootEntry);
    }
    if (readRootDirectoryInformation) {
        rootDirectoryInformation();
    }
}