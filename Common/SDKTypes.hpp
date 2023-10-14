//
//  SDKTypes.hpp
//  Common
//
//  Created by Arun A on 26/09/23.
//
#ifndef SDKTypes_hpp
#define SDKTypes_hpp

#define PLATFORM_MAC        2
#define PLATFORM_UNIX       3
#define PLATFORM_ANDROID    4

#if defined(_WIN32)
    #error "Not supported"
#elif defined(__APPLE__)
    #define PLATFORM PLATFORM_MAC
#elif defined(ANDROID)
    #define PLATFORM PLATFORM_ANDROID
#else
    #define PLATFORM PLATFORM_UNIX
#endif

#define DECLSPEC

#define GSDK_UDP_DEFAULT_PORT 5000


#define GX_DELETE(x)        if(x){delete x; x=NULL;}
#define GX_DELETE_ARY(x)    if(x){delete [] x; x=NULL;}

#define GX_ABS(v) std::abs(v)

#define UNUSED(x)   (void)x

#define __DEFAULT_LOG_TAG__ "gsdkLog"

extern "C" DECLSPEC void PrintCommonInfo();

#define LOG_LEVEL_0 0
#define LOG_LEVEL_1 1
#define LOG_LEVEL_2 2
#define LOG_LEVEL_3 3


#define LOG_LEVEL LOG_LEVEL_0
extern "C" DECLSPEC void DEBUG_PRINT(int logLevel, const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_WARN(const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_ERROR(const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_ASSERT(const char* tag, bool condition, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_IMPORTANT(const char* tag, const char* format, ...);
#endif /* SDKTypes_hpp */
