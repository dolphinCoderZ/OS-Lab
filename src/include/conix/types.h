#ifndef CONIX_TYPES_H
#define CONIX_TYPES_H

#define EOF -1
#define NULL ((void *)0)
#define EOS '\0'

#define __cplusplus
#define bool _Bool
#define true 1
#define false 0

#define _packed __attribute__((packed))

#define _ofp __attribute__((optimize("omit-frame-pointer")))

typedef unsigned int size_t;

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef int32 pid_t;
typedef int32 dev_t;

typedef uint32 time_t;
typedef uint32 idx_t;

typedef uint16 mode_t;

typedef int32 fd_t;

typedef enum std_fd_t {
  stdin,
  stdout,
  stderr,
} std_fd_t;

typedef int32 off_t;

#endif