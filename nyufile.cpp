#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "dataStructers.h"
#include <stdio.h>

using namespace std;

bool sha1 = false;
bool readFileSystemInformation = false; // TODO: Init to true for M1 due to getopt
bool readRootDirectoryInformation = false; // TODO: Init to true for M1 due to getopt
bool readRecoverContiguousFile = false; // TODO: Init to true for M1 due to getopt
char *diskName;
struct BootEntry bootEntry;
string fileName;

int initialize() {
    int disk = open(diskName, O_RDONLY);
    pread(disk, &bootEntry, sizeof(bootEntry), 0);
    return disk;
}

void fileSystemInformation() {
    cout << "Number of FATs = " << (int) bootEntry.BPB_NumFATs << endl;
    cout << "Number of bytes per sector = " << bootEntry.BPB_BytsPerSec << endl;
    cout << "Number of sectors per cluster = " << (int) bootEntry.BPB_SecPerClus << endl;
    cout << "Number of reserved sectors = " << (int) bootEntry.BPB_RsvdSecCnt << endl;
}

void recoverContiguousFile() {
    cout << "I am here " << fileName << endl;
}

long getClusterSize() {
    return bootEntry.BPB_BytsPerSec * bootEntry.BPB_SecPerClus;
}

long getNextCluster(long cluster, int fp) {
    uint32_t next;
    pread(fp, &next, 4, bootEntry.BPB_RsvdSecCnt * bootEntry.BPB_BytsPerSec + cluster * 4);
    return next;
}

long getAddrByCluster(long cluster) {
    return (bootEntry.BPB_RsvdSecCnt + bootEntry.BPB_NumFATs * bootEntry.BPB_FATSz32) * bootEntry.BPB_BytsPerSec +
           (cluster - bootEntry.BPB_RootClus) * getClusterSize();
}

void rootDirectoryInformation(int disk) {
    struct DirEntry dirEntry;
    long cluster = bootEntry.BPB_RootClus;
    int offset = 0, num = 0, i;
    long position;
    while (cluster < 0x0ffffff7 && pread(disk, &dirEntry, sizeof(DirEntry), getAddrByCluster(cluster) + offset) >= 0) {
        if (dirEntry.DIR_Name[0] == 0) {
            break;
        }
        string dirNameInDirectory = "";
        for (i = 0; i < 11; i++) {
            if (i == 8 && dirEntry.DIR_Name[i] != ' ') {
                dirNameInDirectory += ".";
            }
            if (dirEntry.DIR_Name[i] == 0xe5) {
                continue;
            } else if (dirEntry.DIR_Name[i] == ' ') {
                continue;
            } else {
                dirNameInDirectory += dirEntry.DIR_Name[i];
            }
        }
        cout << dirNameInDirectory;
        if ((dirEntry.DIR_Attr & 0b10000) != 0) {
            cout << "/";
        }
        position = dirEntry.DIR_FstClusHI * 65535 + dirEntry.DIR_FstClusLO;
        cout << " (size = " << dirEntry.DIR_FileSize << ", starting cluster = " << position << ")" << endl;
        offset += sizeof(DirEntry);
        if (offset == getClusterSize()) {
            cluster = getNextCluster(cluster, disk);
            offset = 0;
        }
        num++;
    }
    cout << "Total number of entries = " << num << endl;
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
    opterr = 0;
    if (argc <= 1) {
        printUsage();
        return 0;
    }
    diskName = argv[1];

    while ((c = getopt(argc, argv, "ilr:R:s:")) != -1) {
        switch (c) {
            case 'i':
                readFileSystemInformation = true;
                break;
            case 'l':
                readRootDirectoryInformation = true;
                break;
            case 'r':
                readRecoverContiguousFile = true;
                fileName = optarg;
                break;
            case 'R':
                fileName = optarg;
                break;
            case 's':
                sha1 = true;
                break;
            default:
                printUsage();
                return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    parseInput(argc, argv);
    int disk = initialize();
    if (readFileSystemInformation) {
        fileSystemInformation();
    }
    if (readRootDirectoryInformation) {
        rootDirectoryInformation(disk);
    }
    if (readRecoverContiguousFile) {
        recoverContiguousFile();
    }
}