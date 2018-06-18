#ifndef ROMUTILS_H
#define ROMUTILS_H

#include <vector>
#include <string>
#include <cstdio>  //include definition for FILE
#include <cstring>

#include "WL4Constants.h"

namespace ROMUtils
{
    // Global variables
    extern unsigned char *CurrentFile;
    extern int CurrentFileSize;
    extern char ROMFilePath[260]; //the MAX_PATH in Windows is 260, perhaps we should use a bigger number

    // Global functions
    int IntFromData(int address);
    int PointerFromData(int address);
    unsigned char *LayerRLEDecompress(int address, int outputSize);
    long LayerRLECompress(int _layersize, unsigned char *LayerData, unsigned char *OutputCompressedData);
    int FindSpaceInROM(int NewDataLength);
    int SaveTemp(unsigned char *tmpData, int pointerAddress, int dataLength);
    void SaveFile();
}

#endif // ROMUTILS_H
