# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("qunityplugin")

# Add the directory to the include path
include_directories("../../../../../../networkcommon/source/ev"
        "../../../../../../networkcommon/source/uv"
        "../../../../../../networkcommon/source/quiche"
        "../../../../../../common/uthash"
        "../../../../../../common"
        )
#message("debug msg"${CMAKE_CURRENT_SOURCE_DIR})


# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.

message("ABI--> " ${ANDROID_ABI})
add_library(quiche STATIC IMPORTED)
set_target_properties(quiche PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../../../../../../networkcommon/libs/android/${ANDROID_ABI}/libquiche.a)

add_library(ev SHARED IMPORTED)
set_target_properties(ev PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../../../../../../networkcommon/libs/android/${ANDROID_ABI}/libev.so)

add_library(uv SHARED IMPORTED)
set_target_properties(uv PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../../../../../../networkcommon/libs/android/${ANDROID_ABI}/libuv.so)

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()

# Add a definition globally
add_definitions(-DPROD_BUILD=1 -DUSE_PTHREAD=1 -DLOG_LEVEL=2)

# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native-lib.cpp
        native-qh3client.cpp
        ../../../../../../qunityplugin/source/qunitysdk.cpp
        ../../../../../../qh3client/qh3client/qh3client_helper.cpp
        ../../../../../../qh3client/qh3client/qh3client.cpp
        ../../../../../../qclient/source/qnetworkclient.cpp
        ../../../../../../qh3client/qh3client/qh3client-android.cpp
        ../../../../../../networkcommon/source/essentials.cpp
        ../../../../../../common/sdktypes.cpp
        ../../../../../../common/timer.cpp
        )

# Set the output directory for the shared library
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI})

#        # List C/C++ source files with relative paths to this CMakeLists.txt.
#        native-lib.cpp)
# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        log
        quiche
        ev
        uv
        ${ZLIB_LIBRARIES}
        )