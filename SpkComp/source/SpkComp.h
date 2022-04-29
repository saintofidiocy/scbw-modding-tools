#include <stdint.h>

// version constants
#define VER_MAJOR	  1
#define VER_MINOR	  1
#define VER_RELEASE	0
#define VER_YEAR    "2022"

// get version as strings
#define STR_HELPER(x) #x
#define MAKESTR(x) STR_HELPER(x)
#define VSTR_MAJ MAKESTR(VER_MAJOR)
#define VSTR_MIN MAKESTR(VER_MINOR)
#define VSTR_RLS MAKESTR(VER_RELEASE)
#define VSTR     VSTR_MAJ "." VSTR_MIN "." VSTR_RLS

// generic types
typedef uint8_t  u8;
typedef  int8_t  s8;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint32_t u32;
typedef  int32_t s32;
typedef enum {false, true} bool;

u32 crcNext(u32 crc, u8 data);
