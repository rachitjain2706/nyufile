#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "dataStructers.h"
#include <cstring>
#include <openssl/sha.h>
#include <vector>
#include <stdio.h>
#include <cmath>
#include <iomanip>
#include <sstream>

/*#define SHA_DIGEST_LENGTH 40*/

using namespace std;

bool readFileSystemInformation = false; // TODO: Init to true for M1 due to getopt
bool readRootDirectoryInformation = false; // TODO: Init to true for M1 due to getopt
bool readRecoverContiguousFile = false; // TODO: Init to true for M1 due to getopt
bool sha1 = false; // TODO: Init to true for M1 due to getopt
char *diskName;
struct BootEntry bootEntry;
char *fileName;
char *shaHash;

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

long getClusterSize() {
    return bootEntry.BPB_BytsPerSec * bootEntry.BPB_SecPerClus;
}

long getNextCluster(long cluster, int fp) {
    uint32_t next;
    pread(fp, &next, 4, bootEntry.BPB_RsvdSecCnt * bootEntry.BPB_BytsPerSec + cluster * 4);
    return next;
}

long getAddressByCluster(long cluster) {
    return (bootEntry.BPB_RsvdSecCnt + bootEntry.BPB_NumFATs * bootEntry.BPB_FATSz32) * bootEntry.BPB_BytsPerSec +
           (cluster - bootEntry.BPB_RootClus) * getClusterSize();
}

long rootDirectoryIndex(int disk, long startingCluster) {
    struct DirEntry dirEntry;
    long cluster = bootEntry.BPB_RootClus;
    long offset = 0, num = 0, i;
    long position;
    while (cluster < 0x0ffffff7 &&
           pread(disk, &dirEntry, sizeof(DirEntry), getAddressByCluster(cluster) + offset) >= 0) {
        if (dirEntry.DIR_Name[0] == 0) {
            break;
        }
        string dirNameInDirectory;
        for (i = 0; i < 11; i++) {
            if (i == 8 && dirEntry.DIR_Name[i] != ' ') {
                dirNameInDirectory += ".";
            }
            if (dirEntry.DIR_Name[i] == 0xe5) { // Deleted file
                dirNameInDirectory += "?";
                continue;
            } else if (dirEntry.DIR_Name[i] == ' ') {
                continue;
            } else {
                dirNameInDirectory += dirEntry.DIR_Name[i];
            }
        }

        position = dirEntry.DIR_FstClusHI * 65535 + dirEntry.DIR_FstClusLO;
        if (position == startingCluster) {
            return num;
        }
        num++;
        offset += sizeof(DirEntry);
        if (offset == getClusterSize()) {
            cluster = getNextCluster(cluster, disk);
            offset = 0;
        }
    }
    cout << "Total number of entries = " << num << endl;
}

void rootDirectoryInformation(int disk) {
    struct DirEntry dirEntry;
    long cluster = bootEntry.BPB_RootClus;
    int offset = 0, num = 0, i;
    long position;
    while (cluster < 0x0ffffff7 &&
           pread(disk, &dirEntry, sizeof(DirEntry), getAddressByCluster(cluster) + offset) >= 0) {
        if (dirEntry.DIR_Name[0] == 0) {
            break;
        }
        string dirNameInDirectory;
        for (i = 0; i < 11; i++) {
            if (i == 8 && dirEntry.DIR_Name[i] != ' ') {
                dirNameInDirectory += ".";
            }
            if (dirEntry.DIR_Name[i] == 0xe5) { // Deleted file
                dirNameInDirectory += "?";
                continue;
            } else if (dirEntry.DIR_Name[i] == ' ') {
                continue;
            } else {
                dirNameInDirectory += dirEntry.DIR_Name[i];
            }
        }
        if (dirNameInDirectory[0] != '?') {
            cout << dirNameInDirectory;
            if ((dirEntry.DIR_Attr & 0b10000) != 0) {
                cout << "/";
            }
            position = dirEntry.DIR_FstClusHI * 65535 + dirEntry.DIR_FstClusLO;
            cout << " (size = " << dirEntry.DIR_FileSize << ", starting cluster = " << position << ")" << endl;
            num++;
        }
        offset += sizeof(DirEntry);
        if (offset == getClusterSize()) {
            cluster = getNextCluster(cluster, disk);
            offset = 0;
        }
    }
    cout << "Total number of entries = " << num << endl;
}

DirEntry getFileEntry(int fp, char *filename, long cluster) {
    auto *convertedName = (unsigned char *) (filename);
    int offset = 0;
    int i;
    DirEntry *dirEntry;
    vector<DirEntry> answerVec;
    DirEntry copyEntry;
    dirEntry = static_cast<DirEntry *>(malloc(sizeof(DirEntry)));
    char name[11];
    int num = 0;
    for (i = 0; i < 11; i++) {
        name[i] = ' ';
    }
    if (strstr(filename, ".") != nullptr) {
        filename = strtok(filename, ".");
        memcpy(name, filename, strlen(filename));
        filename = strtok(nullptr, ".");
        memcpy(name + 8, filename, strlen(filename));
    } else {
        memcpy(name, filename, strlen(filename));
    }
    while (cluster < 0x0ffffff7 && pread(fp, dirEntry, sizeof(DirEntry), getAddressByCluster(cluster) + offset) >= 0) {
        if (dirEntry->DIR_Name[0] == 0) {
            if (sha1) {
                cout << fileName << ": file not found" << endl;
                return copyEntry;
            }
            if (num == 0) {
                cout << fileName << ": file not found" << endl;
            } else if (num == 1) {
                /*answerVec.at(0)->DIR_Name[0] = name[0];*/
                cout << "DEBUG direntry: " << answerVec.at(0).DIR_FstClusHI << "-" << answerVec.at(0).DIR_FstClusLO
                     << endl;
                return answerVec.at(0);
            } else {
                cout << fileName << ": multiple candidates found" << endl;
            }
            return copyEntry;
        }
        if (dirEntry->DIR_Name[0] == 0xe5) {
            for (i = 1; i < 11; i++) {
                if (name[i] != dirEntry->DIR_Name[i]) {
                    break;
                }
            }
            if (i == 11) {
                if (sha1) {
                    unsigned char resHash[SHA_DIGEST_LENGTH];
                    dirEntry->DIR_Name[0] = name[0];
                    /*cout << "DEBUG DIR_Name: " << dirEntry->DIR_Name << endl;*/
                    /*SHA1(dirEntry->DIR_Name, sizeof(dirEntry->DIR_Name) - 1, resHash);*/
                    SHA1(convertedName, sizeof(convertedName), resHash);
                    /*cout << convertedName << endl;*/
                    auto *convertedShaHash = (unsigned char *) (shaHash);
                    cout << "DEBUG resHash: ";
                    for (int cp = 0; cp < 20; cp++) {
                        printf("%02x", resHash[cp]);
                    }
                    cout << endl;
                    if (resHash == convertedShaHash) {
                        return copyEntry;
                    }
                }
                /*copyDirEntry = dirEntry;*/
                cout << "DEBUG before vector direntry: " << dirEntry->DIR_FstClusHI << "-" << dirEntry->DIR_FstClusLO
                     << endl;
                copyEntry.DIR_FstClusHI = dirEntry->DIR_FstClusHI;
                copyEntry.DIR_FstClusLO = dirEntry->DIR_FstClusLO;
                copyEntry.DIR_FileSize = dirEntry->DIR_FileSize;
                answerVec.push_back(copyEntry);
                num++;
            }
        }
        offset += sizeof(DirEntry);
        if (offset == getClusterSize()) {
            cout << "DEBUG getFileEntry()" << endl;
            cluster = getNextCluster(cluster, fp);
            offset = 0;
        }
    }
    /*free(dirEntry);*/
    return copyEntry;
}

void updateFAT(long cluster, int numClusters, int disk, FILE *fp) {
    /*uint32_t startingPointer = getNextCluster(cluster, disk);*/
    uint32_t startingPointer = bootEntry.BPB_RsvdSecCnt * bootEntry.BPB_BytsPerSec;
    cout << "DEBUG startingPointer: " << startingPointer << endl;
    while (numClusters >= 1) {
        fseek(fp, startingPointer + cluster * 4, SEEK_SET);
        char *str = (char *) malloc(3 * sizeof(char));
        str[1] = 0;
        str[0] = (char) (cluster + 1);
        if (numClusters == 1) {
            str = (char *) malloc(5 * sizeof(char));
            str[0] = 0xff;
            str[3] = 0x0f;
            str[2] = 0xff;
            str[1] = 0xff;
        }
        fputs(str, fp);
        cout << "DEBUG numClusters: " << numClusters << endl;
        numClusters--;
        cluster++;
    }

}

void recoverContiguousFile(int disk) {
    long cluster;
    int fileSize;
    DirEntry *dirEntry;
    char *temp_fileName = static_cast<char *>(malloc(sizeof(fileName) + 1));
    strcpy(temp_fileName, fileName);
    cluster = bootEntry.BPB_RootClus;
    DirEntry de = getFileEntry(disk, temp_fileName, cluster);
    dirEntry = &de;
    if (dirEntry == nullptr) {
        return;
    }
    cluster = dirEntry->DIR_FstClusHI * 65535 + dirEntry->DIR_FstClusLO;
    /*cout << "DEBUG cluster: " << cluster << endl;*/
    fileSize = dirEntry->DIR_FileSize;
    if (cluster != 0 && getNextCluster(cluster, disk) != 0) {
        return;
    }
    cout << fileName << ": successfully recovered";
    if (sha1) {
        cout << " with SHA-1" << endl;
    } else {
        cout << endl;
    }
    char *fileBuffer;
    fileBuffer = static_cast<char *>(malloc(fileSize));
    pread(disk, fileBuffer, fileSize, getAddressByCluster(cluster));
    cout << "DEBUG file buffer: " << fileBuffer << endl;
    FILE *fp = fopen(diskName, "r+");
    fseek(fp, getAddressByCluster(cluster), SEEK_SET);
    unsigned char fileChar;
    fread(&fileChar, 1, 1, fp);
    long index = rootDirectoryIndex(disk, cluster);
    /*cout << "DEBUG index: " << index << endl;*/
    long fileAddress =
            getAddressByCluster(bootEntry.BPB_RootClus) + (index * sizeof(DirEntry)) +
            1;
    fseek(fp, fileAddress, SEEK_SET);
    fseek(fp, -1, SEEK_CUR);
    fputc(fileName[0], fp);
    int numClusters = ceil(dirEntry->DIR_FileSize / bootEntry.BPB_BytsPerSec);
    if (numClusters > 1) {
        updateFAT(cluster, numClusters, disk, fp);
    } else {
        uint32_t startingPointer = bootEntry.BPB_RsvdSecCnt * bootEntry.BPB_BytsPerSec;
        fseek(fp, startingPointer + cluster * 4, SEEK_SET);
        char *str = (char *) malloc(5 * sizeof(char));
        str[0] = 0xff;
        str[3] = 0x0f;
        str[2] = 0xff;
        str[1] = 0xff;
        fputs(str, fp);
    }
    /*close(disk);*/
    fclose(fp);
}

void fileRecoverySHA() {
    cout << "DEBUG SHA-1: " << shaHash << endl;
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
                /*c91761a2cc1562d36585614c8c680ecf5712e875 - GANG.TXT*/
                /*4e0a5acd4f36681b2df7e097a82c4f4b74b3f096 - TANG.TXT*/
                shaHash = optarg;
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
        recoverContiguousFile(disk);
    }
    if (sha1) {
        fileRecoverySHA();
    }
}