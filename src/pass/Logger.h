#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>
#include <algorithm>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/DebugInfo.h>
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define INFO_SIZE 5
#define TYPE_SIZE 3
short NEW_FILE_MASK = 0x8000;

typedef enum {
    ARITHMETIC_FP = 0,
    POINTER,
    ARITHMETIC_FIX,
    CONTROL_LOOP,
    CONTROL_BRANCH,  
    UNKNOWN_INJ
} INJ_TYPES;

typedef enum {
    RESULT = 0,
    VALUE = RESULT,
    ADDRESS,
    UNKNOWN_INJ_TYPE = 30
} INJ_INFO_TYPES;

class LogFile
{
  public:
    LogFile(std::string srcName, unsigned long currentSite, std::string suffix = ".LLVM.bin", int bufSize = 8192, char version = 1) {
        init(srcName, currentSite, suffix, bufSize, version);
    }
    //LogFile(char* filename, string::string suffix = ".LLVM.txt", int bufSize = 8192, char version) {
    //    init(filename, bufSize, versions);
    //}

    void init(std::string srcName, unsigned long currentSite, std::string suffix, int bufSize, char version) {
        srcFile = srcName;
        outfile.open(srcName+suffix, std::ios::out | std::ios::binary);
        buffer = new char[bufSize];
        this->bufSize = bufSize;
        currSize = 0;
        oldSite = currentSite -1;
        logFileHeader(version);      
    }

    ~LogFile() {
        // close();
        delete [] buffer;
    }

    void logFunctionHeader(unsigned long site, std::string name)
    {
        // make sure we have enough room
        if (currSize + name.size() + 1 + sizeof(site) > bufSize)
            write();

        //function name
        assert(name.size() <= 255 && "Logging function with name >255.\n");
        
        // DUMMY operand flag
        buffer[currSize++] = 255;
        buffer[currSize++] = (char) name.size();
        memcpy(buffer+currSize, name.c_str(), std::min((int)name.size(), (1 << 8) -1));
        currSize += std::min((int)name.size(), (1 << 8) -1);

        // current fault site index
        oldSite = site - 1;
        char* ptr = (char*) &site;
        for (unsigned i=0; i < sizeof(site); i++) {
            buffer[currSize++] = *(ptr + i);
        }

    }
    void logInst(unsigned long site, int injType, int comment, Instruction* I)
    {
        // make sure we have enough room
        if (currSize + 5 > bufSize) // 5 bytes compressed data
            write();
        
        // operand
        buffer[currSize++] = (char) I->getOpcode(); // Assumes < 255 insts
        
        assert(oldSite + 1 == site && "Sites differ > 1.\n");
        // Type and info
        char type_info = (getType(injType) << INFO_SIZE) | getInfo(comment);
        buffer[currSize++] = type_info;
        oldSite = site;

        // location in file
        logFileLocation(I);
    } 
    inline bool needsWriting() { return currSize > 0; }
    void write() {
        if (needsWriting()) {
            outfile.write(buffer, currSize);
            currSize = 0;
        }
    }
    void close() {
        if (outfile.is_open()) {
            if (needsWriting())
                write();
                outfile.close();
        }
    }

  private:
    void logFileHeader(char version)
    {
        // file version
        buffer[currSize++] = version;
        
        //source file properties
        oldFile = srcFile;
        unsigned short size = srcFile.size();
        char* ptr = (char*)&(size);
        buffer[currSize++] = *ptr;
        buffer[currSize++] = *(ptr+1);
        memcpy(buffer+currSize, srcFile.c_str(), std::min((int)size, (1 << 16) -1));
        currSize += std::min((int)size, (1 << 16) -1);
    }

    unsigned char getType(int injType)
    {
        if (injType < ARITHMETIC_FP && injType > CONTROL_BRANCH)
            injType = UNKNOWN_INJ;
        return (unsigned char)injType;
    }
    
    unsigned char getInfo(int comment)
    {
        if (comment < ARITHMETIC_FP && comment > UNKNOWN_INJ_TYPE)
            comment = UNKNOWN_INJ_TYPE;
        return (unsigned char)comment;
    }
    void logFileLocation(Instruction* I)
    {
        unsigned short size = 0;
        unsigned short lineNum = 0;
        std::string location = "";
        MDNode* N = I->getMetadata("dbg");
        
        if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
            DILocation Loc(N);
            location = Loc.getDirectory().str() + "/" + Loc.getFilename().str();
            lineNum = Loc.getLineNumber();
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
            DILocation* Loc = oldI->getDebugLoc();
            location = Loc->getDirectory()->str()  + "/" + Loc->getFilename()->str();
            lineNum = Loc->getLineNumber();
#endif
            if (oldFile != location) {
                size = location.size();
                size |= NEW_FILE_MASK;//0x80; // set MSB
            }
        }
        else /* no debugging information */ {
            location = "__NF";
            if (oldFile != "__NF") {
                size = 4 | NEW_FILE_MASK;//0x80;
            }
            else /* no prev dbg info */ {
                // nothing...
            }
        }
        // file size if new file
        if (oldFile != location) {
            char* ptr = (char*)&(size);
            buffer[currSize++] = *ptr;
            buffer[currSize++] = *(ptr+1);
        }

        // line number
        char* ptr = (char*)&(lineNum);
        buffer[currSize++] = *ptr;
        buffer[currSize++] = *(ptr+1);

        // file name if need be
        if (size & NEW_FILE_MASK) {
            if (currSize + location.size() > bufSize)
                write();
            
            memcpy(buffer+currSize, location.c_str(), std::min((int)location.size(), (1 << 16) -1));
            currSize += std::min((int)location.size(), (1 << 16) -1);
        }
        oldFile = location;
    }


    // data
    ofstream outfile;
    std::string srcFile;
    std::string oldFile;
    unsigned long oldSite;
    char* buffer;
    unsigned  bufSize;
    unsigned currSize;
};
#endif

