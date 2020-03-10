# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


add_library( # Sets the name of the library.
        audio-player

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        audio_player_jni.cpp
        AudioPlayer.cpp
        AudioPacketQueue.cpp
        SLManager.cpp)
include_directories(${CMAKE_SOURCE_DIR}/include)
set(JNILIBS_SRC ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})
message(${ANDROID_ABI})
add_library(
        avcodec
        SHARED
        IMPORTED
)

set_target_properties(
        avcodec
        PROPERTIES IMPORTED_LOCATION
        ${JNILIBS_SRC}/libavcodec.so
)

add_library(
        avfilter
        SHARED
        IMPORTED
)

set_target_properties(
        avfilter
        PROPERTIES IMPORTED_LOCATION
        ${JNILIBS_SRC}/libavfilter.so
)

add_library(
        avformat
        SHARED
        IMPORTED
)

set_target_properties(
        avformat
        PROPERTIES IMPORTED_LOCATION
        ${JNILIBS_SRC}/libavformat.so
)

add_library(
        avutil
        SHARED
        IMPORTED
)

set_target_properties(
        avutil
        PROPERTIES IMPORTED_LOCATION
        ${JNILIBS_SRC}/libavutil.so
)

add_library(
        swresample
        SHARED
        IMPORTED
)

set_target_properties(
        swresample
        PROPERTIES IMPORTED_LOCATION
        ${JNILIBS_SRC}/libswresample.so
)

add_library(
        swscale
        SHARED
        IMPORTED
)

set_target_properties(
        swscale
        PROPERTIES IMPORTED_LOCATION
        ${JNILIBS_SRC}/libswscale.so
)

#add_library(
#        openssl
#        SHARED
#        IMPORTED
#)
#
#set_target_properties(
#        openssl
#        PROPERTIES IMPORTED_LOCATION
#        ${JNILIBS_SRC}/libssl.so
#)
#
#add_library(
#        crypto
#        SHARED
#        IMPORTED
#)
#
#set_target_properties(
#        crypto
#        PROPERTIES IMPORTED_LOCATION
#        ${JNILIBS_SRC}/libcrypto.so
#)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        audio-player
        avcodec
        avfilter
        avformat
        avutil
        swresample
        swscale
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        OpenSLES)