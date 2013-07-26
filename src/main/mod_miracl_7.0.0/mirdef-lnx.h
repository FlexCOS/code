/*
 *   MIRACL compiler/hardware definitions - mirdef.h
 */
#define MR_LITTLE_ENDIAN
#define MIRACL 32
#define mr_utype int
#define MR_IBITS 32
#define MR_LBITS 64
#define mr_unsign32 unsigned int
#define mr_unsign64 unsigned long
#define mr_dltype long
#define MR_DLTYPE_IS_LONG
#define MR_STATIC 8
#define MR_ALWAYS_BINARY
#define MR_NOASM
#define MR_NO_STANDARD_IO
#define MR_NO_FILE_IO
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_COMBA 8
#define MR_BITSINCHAR 8
//#define MR_AFFINE_ONLY
//#define MR_NOSUPPORT_COMPRESSION
#define MR_NOKOBLITZ
#define MR_NO_SS
