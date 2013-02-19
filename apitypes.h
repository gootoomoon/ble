#ifndef __APITYPES_H__
#define __APITYPES_H__

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#define ALIGNED __attribute__((aligned(0x4)))
#else
#define PACKED
#define ALIGNED
#endif

typedef unsigned char      uint8;
typedef signed char        int8;
typedef unsigned short int uint16;
typedef signed short int   int16;
typedef unsigned long int  uint32;

typedef struct PACKED
{
    uint8 byte[6];
} ble_device_address_t;

#endif
