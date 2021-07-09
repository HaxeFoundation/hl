cmake_minimum_required(VERSION 3.1)

set(HL_VERSION_MAJOR 1)
set(HL_VERSION_MINOR 12)
set(HL_VERSION_PATCH 0)
set(HL_VERSION ${HL_VERSION_MAJOR}.${HL_VERSION_MINOR}.${HL_VERSION_PATCH})

cmake_policy(SET CMP0042 NEW)

if(WIN32)
    project(hashlink C CXX) # C++ required for directx
else()
    project(hashlink C)
endif()

include(GNUInstallDirs)
include(FindPkgConfig)
include(CTest)

# force Unicode over Multi-byte
if(MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/other/cmake
)

if(CMAKE_VERSION VERSION_LESS "3.1")
    set(CMAKE_C_FLAGS "-std=c11 ${CMAKE_C_FLAGS}")
else()
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# put output in "bin"

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# avoid the extra "Debug", "Release" directories
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
endforeach()

include_directories(
    src
    include
    include/pcre
)

file(GLOB pcre_srcs
    include/pcre/*.c
)
file(GLOB std_srcs
    src/std/array.c
    src/std/buffer.c
    src/std/bytes.c
    src/std/cast.c
    src/std/date.c
    src/std/error.c
    src/std/file.c
    src/std/fun.c
    src/std/maps.c
    src/std/math.c
    src/std/obj.c
    src/std/random.c
    src/std/regexp.c
    src/std/socket.c
    src/std/string.c
    src/std/sys.c
    src/std/track.c
    src/std/types.c
    src/std/ucs2.c
    src/std/thread.c
    src/std/process.c
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    list(APPEND std_srcs
        include/mdbg/mdbg.c
        include/mdbg/mach_excServer.c
        include/mdbg/mach_excUser.c
    )
endif()

if(ANDROID)
    list(APPEND std_srcs
        src/std/sys_android.c
    )
endif()

if(IOS_PLATFORM)
    list(APPEND std_srcs
        src/std/sys_ios.m
    )
endif()

add_library(libhl SHARED
    ${pcre_srcs}
    src/gc.c
    ${std_srcs}
)

set(public_headers
    src/hl.h
    src/hlc.h
)

set_target_properties(libhl
    PROPERTIES
    PUBLIC_HEADER "${public_headers}"
)

if(WIN32)
    set_target_properties(libhl
        PROPERTIES
        OUTPUT_NAME libhl
    )
else()
    set_target_properties(libhl
        PROPERTIES
        OUTPUT_NAME hl
    )
endif()

set_target_properties(libhl
    PROPERTIES
    VERSION ${HL_VERSION}
    SOVERSION ${HL_VERSION_MAJOR}
    COMPILE_DEFINITIONS "_USRDLL;LIBHL_EXPORTS"
)

add_executable(hl
    src/code.c
    src/jit.c
    src/main.c
    src/module.c
    src/debugger.c
	src/profile.c
)

target_link_libraries(hl libhl)

if(WIN32)
    target_link_libraries(libhl ws2_32 user32)
    target_link_libraries(hl user32)
else()
    target_link_libraries(libhl m dl pthread)
endif()

if(BUILD_TESTING)

    find_program(
        HAXE_COMPILER
        haxe
    )

    #####################
    # hello.hl

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
        COMMAND ${HAXE_COMPILER}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main HelloWorld
    )
    add_custom_target(hello.hl ALL
        DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
    )

    #####################
    # threads.hl

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
        COMMAND ${HAXE_COMPILER}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main Threads
    )
    add_custom_target(threads.hl ALL
        DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
    )

    #####################
    # hello.c

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello/hello.c
        COMMAND ${HAXE_COMPILER}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello/hello.c
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main HelloWorld
    )
    add_executable(hello
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello/hello.c
    )
    set_target_properties(hello
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello
    )
    target_include_directories(hello
        PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello
    )
    target_link_libraries(hello
        libhl
    )

    #####################
    # threads.c

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads/threads.c
        COMMAND ${HAXE_COMPILER}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads/threads.c
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main Threads
    )
    add_executable(threads
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads/threads.c
    )
    set_target_properties(threads
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads
    )
    target_include_directories(threads
        PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads
    )
    target_link_libraries(threads
        libhl
    )

    #####################
    # Tests

    add_test(NAME hello.hl
        COMMAND hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
    )
    add_test(NAME threads.hl
        COMMAND hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
    )
    add_test(NAME hello
        COMMAND hello
    )
    add_test(NAME threads
        COMMAND threads
    )
    add_test(NAME version
        COMMAND hl --version
    )
    set_tests_properties(version
        PROPERTIES
        PASS_REGULAR_EXPRESSION "${HL_VERSION}"
    )

endif()

#####################
# Packaging

set(CPACK_OUTPUT_FILE_PREFIX ${OUTPUT_DIR})

set(CPACK_PACKAGE_VENDOR "Haxe Foundation")
set(CPACK_PACKAGE_VERSION_MAJOR ${HL_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HL_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HL_VERSION_PATCH})

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(HDLL_DESTINATION
    ${CMAKE_INSTALL_LIBDIR}
)

install(
    TARGETS
        hl
        libhl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# uninstall target
# https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/other/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()


if(
    NOT DEFINED ENV{APPVEYOR_REPO_TAG_NAME}
    AND
    DEFINED ENV{APPVEYOR_REPO_COMMIT}
)
    string(SUBSTRING $ENV{APPVEYOR_REPO_COMMIT} 0 7 short_commit)
endif()

if(NOT DEFINED ENV{TRAVIS_TAG} AND DEFINED ENV{TRAVIS_COMMIT})
    string(SUBSTRING $ENV{TRAVIS_COMMIT} 0 7 short_commit)
endif()

if(DEFINED short_commit)
    set(CPACK_PACKAGE_VERSION "${HL_VERSION}+${short_commit}")
endif()


include(CPack)

add_subdirectory(libs)
