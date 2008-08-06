#ifndef _EDTYPES_H_
#define _EDTYPES_H_

#define EDEC_OK         0 
#define EDEC_NOMEMORY  -16
#define EDEC_READFAIL  -17
#define EDEC_WRITEFAIL -18
#define EDEC_BADFORMAT -19

//#ifdef WIN32
#define UNIT32
typedef DWORD UNIT;
#define UNITSIZE       32
//#else
//#define UNIT16
//typedef WORD UNIT;
//#define UNITSIZE       16
//#endif

typedef UNIT *UNITPTR;

#define MIN_KEY_BITS  384
#define MAX_KEY_BITS 2048

/*	MAX_BIT_PRECISION is upper limit that assembly primitives can handle.
	It must be less than 32704 bits, or 4088 bytes.  It should be an
	integer multiple of UNITSIZE*2.
*/
#define MAX_BIT_PRECISION  (MAX_KEY_BITS+(2*UNITSIZE))
#define MAX_BYTE_PRECISION (MAX_BIT_PRECISION/8)
#define MAX_UNIT_PRECISION (MAX_BIT_PRECISION/UNITSIZE)

class CPubKey
{
public:

    UNIT n[MAX_UNIT_PRECISION];
    UNIT e[MAX_UNIT_PRECISION];
};

class CPrivKey
{
public:

    UNIT p[MAX_UNIT_PRECISION];
    UNIT q[MAX_UNIT_PRECISION];
    UNIT d[MAX_UNIT_PRECISION];
    UNIT u[MAX_UNIT_PRECISION];
};

class CKeyPair : public CPubKey, public CPrivKey
{
};

class CMDCtx;

typedef CMDCtx *HMDCTX;

#endif  // _EDTYPES_H_
