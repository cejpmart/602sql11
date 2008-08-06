//
// PKZIP file format structures, 602zip extension for work with files greater then 4GB
//
// Original PKZIP file format supports only 32 bit values, for original and compressed file size and for
// offsets in zip file. 602zip extension writes additional informations into space for comments or extra
// bytes. Additional data structures are allways introduced with ID value 0x4247343E i.e. string ">4GB"
// folowed by terminating 0 and holds upper 32 bits of huge values. 
//
#include "ZipDefs.h"
#pragma pack(1)
//
// If compressed file is >= 4 GB, or compressed file position in zip archive is >= 4GB, Zip4GB12 structure
// is writen to comment space of ZipEntryInfo
//
struct Zip4GB12
{
    DWORD ID;               // ID - >4GB
    WORD  _1;               // 0  - string terminator
    DWORD ZipSizeHigh;      // Upper bits of compressed size
    DWORD fLengthHigh;      // Upper bits of original size
    DWORD ZipPosHigh;       // Upper bits of file header position in archive
};

typedef struct Zip4GB12 *LPZIP4GB12;
//
// If compressed file is >= 4 GB, Zip4GB34 structure is writen to extra bytes space of ZipEntryHdr
//
struct Zip4GB34
{
    DWORD ID;               // ID - >4GB
    WORD  _1;               // 0  - string terminator
    DWORD ZipSizeHigh;      // Upper bits of compressed size
    DWORD fLengthHigh;      // Upper bits of original size
};

typedef struct Zip4GB34 *LPZIP4GB34;
//
// If archive directory position in zip file >= 4 GB, Zip4GB56 structure is writen to comment space 
// of ZipFileInfo
//
struct Zip4GB56
{
    DWORD ID;               // ID - >4GB
    WORD  _1;               // 0  - string terminator
    DWORD SeekHigh;         // Upper bits of archive directory position in zip file
};

typedef struct Zip4GB56 *LPZIP4GB56;
//
// Zip archive directory entry
//
class ZipEntryInfo
{
public:

    DWORD ID;               // PK12
    WORD  VerMadeBy;        // Zip software version used to compress this file, operating/file system flag
    WORD  VerNeedToExtr;    // Zip software version needed to extract this file
    WORD  Flags;            // Compression and encryption flags
    WORD  Method;           // Compression method
    WORD  fTime;            // Time of last file change
    WORD  fDate;            // Date of last file change
    DWORD CRC;              // CRC
    DWORD ZipSize;          // Lower bits of compressed size
    DWORD fLength;          // Lower bits of original size
    WORD  fNameSz;          // File name length
    WORD  ExtraBytesSz;     // Extra bytes after file name length
    WORD  CommentSz;        // Comment after extra bytes length
    WORD  DiskNO;           // Index of flopy disc on which this file begins
    WORD  iAttr;            // Internal file flags
    DWORD fAttr;            // External file flags
    DWORD ZipPos;           // Lower bits of file header position in archive
    char  fName[1];         // File name

    // Returns original file size
    QWORD GetFileSize()
    {
        QWORD Result = fLength;
        if (VerNeedToExtr == ZVER_4GB && ((LPZIP4GB12)&fName[fNameSz])->ID == P4GB)
            HIDWORD(Result) = ((LPZIP4GB12)&fName[fNameSz])->fLengthHigh;
        return Result;
    }
    // Returns file header position
    UQWORD GetZipPos()
    {
        QWORD Result = ZipPos;
        if (VerNeedToExtr == ZVER_4GB && ((LPZIP4GB12)&fName[fNameSz])->ID == P4GB)
            HIDWORD(Result) = ((LPZIP4GB12)&fName[fNameSz])->ZipPosHigh;
        return Result;
    }              
    // Returns compressed file size
    QWORD GetZipSize()
    {
        QWORD Result = ZipSize;
        if (VerNeedToExtr == ZVER_4GB && ((LPZIP4GB12)&fName[fNameSz])->ID == P4GB)
            HIDWORD(Result) = ((LPZIP4GB12)&fName[fNameSz])->ZipSizeHigh;
        return Result;
    }
    // Returns size of whole directory entry including file name, extra bytes and comment length
    DWORD GetInfoSize()
    {
        return sizeof(ZipEntryInfo) - 1 + fNameSz + ExtraBytesSz + CommentSz;
    }
    // Returns pointer to next directory entry
    ZipEntryInfo *NextEntryInfo()
    {
        return (ZipEntryInfo *)((char *)this + GetInfoSize());
    }
};

typedef ZipEntryInfo *LPZIPENTRYINFO;
//
// Compressed file header
//
struct ZipEntryHdr
{
    DWORD ID;               // PK34
    WORD  Version;          // Zip software version needed to extract this file
    WORD  Flags;            // Compression and encryption flags
    WORD  Method;           // Compression method
    WORD  fTime;            // Time of last file change
    WORD  fDate;            // Date of last file change
    DWORD CRC;              // CRC
    DWORD ZipSize;          // Lower bits of compressed size
    DWORD fLength;          // Lower bits of original size
    WORD  fNameSz;          // File name length
    WORD  ExtraBytes;       // Extra bytes after file name length
    char  fName[1];         // File name

    // Returns whole file header size including file name and extra bytes length
    DWORD  GetHdrSize(){return(sizeof(ZipEntryHdr) - 1 + fNameSz + ExtraBytes);}
    // Returns pointer co compressed file data
    LPBYTE GetData(){return((LPBYTE)this + GetHdrSize());}
    // Returns original file size
    QWORD  GetFileSize()
    {
        QWORD Result = fLength;
        if (ExtraBytes >= sizeof(Zip4GB34) && ((LPZIP4GB34)&fName[fNameSz])->ID == P4GB)
            HIDWORD(Result) = ((LPZIP4GB34)&fName[fNameSz])->fLengthHigh;
        return(Result);
    }
    // Returns compressed file size
    QWORD  GetZipSize()
    {
        QWORD Result = ZipSize;
        if (ExtraBytes >= sizeof(Zip4GB34) && ((LPZIP4GB34)&fName[fNameSz])->ID == P4GB)
            HIDWORD(Result) = ((LPZIP4GB34)&fName[fNameSz])->ZipSizeHigh;
        return(Result);
    }
};

typedef struct ZipEntryHdr *LPZIPENTRYHDR;
//
// Zip archive info
//
struct ZipFileInfo
{
    DWORD   ID;             // PK56
    WORD    DiskNO;         // Current flopy disc index
    WORD    DirDiskNO;      // Index of flopy disc with archive directory
    WORD    LocFileCnt;     // Number of files on this disc
    WORD    FileCnt;        // Number of files in archive
    DWORD   DirSize;        // Archive directory size
    DWORD   DirSeek;        // Lower bits of directory position
    WORD    MsgSz;          // Comment size
    char    Msg[1];         // Comment

    // Returns archive directory position
    QWORD  GetDirSeek()
    {
        QWORD Result = DirSeek;
        if (MsgSz >= sizeof(Zip4GB56) && ((LPZIP4GB56)Msg)->ID == P4GB)
            HIDWORD(Result) = ((LPZIP4GB56)Msg)->SeekHigh;
        return(Result);
    }
};

typedef struct ZipFileInfo *LPZIPFILEINFO;
//
// Data descriptor, in 602zip not used
//
struct ZipDataDescr
{
    DWORD ID;               // PK78
    DWORD CRC;              // CRC
    DWORD ZipSize;          // Compressed file size
    DWORD fLength;          // Original file size
};

typedef struct ZipDataDescr *LPZIPDATADESCR;

#pragma pack()


