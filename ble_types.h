#ifndef __BLE_TYPES_H__
#define __BLE_TYPES_H__

/* Basic definitions */
#ifdef __GNUC__
#define PACKED __attribute__((packed))
#define ALIGNED __attribute__((aligned(0x4)))
#else
#define PACKED
#define ALIGNED
#endif

typedef unsigned char      uint8;
typedef char               int8;
typedef unsigned short int uint16;
typedef short int          int16;
typedef unsigned int       uint32;
typedef int                int32;

typedef int *timer_handle_t;

#endif

