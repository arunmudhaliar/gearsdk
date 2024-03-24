//
//  typex.h
//  common
//
//  Created by Arun A on 03/03/24.
//

#ifndef typex_h
#define typex_h

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef int intx;

#define Q_FACTOR    12
#define FX_ONE        (1<<Q_FACTOR)
#define FF_ONE        4096.0f
#define ITOX(c)        ((c)<<Q_FACTOR)
#define XTOI(c)        ((c)>>Q_FACTOR)
#define FTOX(c)        ((int)((c)*FF_ONE))
#define XTOF(c)        ((float)((c)/FF_ONE))

#define MULTX64(arg1, arg2)    (((__int64_t)arg1*arg2)>>Q_FACTOR)
#define MULTX(arg1, arg2)    (int)(((__int64_t)arg1*arg2)>>Q_FACTOR)
#define DIVX(arg1, arg2)    (int)(ITOX((__int64_t)arg1)/arg2)

#define FX12TO16(arg1)        ((arg1)*(1<<4))
#define FX16TOI(arg1)       ((arg1)>>16)
//#define FX16TO12(arg1)        ((arg1)/(1<<4))
//#define FX12TO16(arg1)        ((arg1)*(1<<16))//for fixed

#define FX_TWO (FX_ONE+FX_ONE)
#define FX_QUATER 1024
#define FX_HALF 2048

#define GX_SWAP_INT(x1, x2)     { int t=x1; x1=x2; x2=t;    }
#define GX_SWAP_FLOAT(x1, x2)   { float t=x1; x1=x2; x2=t;    }

#define GX_MAX_INT  2147483647              // 0x7fffffff;   // max 32-bit signed int
#define GX_MIN_INT (-2147483647 - 1)

#define GX_MAX_FX_INT   524287

#endif /* typex_h */
