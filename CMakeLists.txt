cmake_minimum_required(VERSION 3.16)
project(ksmaudio)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

file(GLOB_RECURSE KSMAUDIO_SOURCES src/*.cpp)
add_library(ksmaudio STATIC ${KSMAUDIO_SOURCES})

target_include_directories(ksmaudio PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(APPLE)
    target_include_directories(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_macOS/bass
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_macOS/bass_fx
    )

    add_library(bass SHARED IMPORTED)
    set_target_properties(bass PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_macOS/bass/libbass.dylib
    )

    add_library(bass_fx SHARED IMPORTED)
    set_target_properties(bass_fx PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_macOS/bass_fx/libbass_fx.dylib
    )

    target_link_libraries(ksmaudio PUBLIC bass bass_fx)
elseif(UNIX)
    target_include_directories(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Linux/bass
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Linux/bass_fx
    )

    add_library(bass SHARED IMPORTED)
    set_target_properties(bass PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Linux/bass/libbass.so
        IMPORTED_NO_SONAME TRUE
    )

    add_library(bass_fx SHARED IMPORTED)
    set_target_properties(bass_fx PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Linux/bass_fx/libbass_fx.so
        IMPORTED_NO_SONAME TRUE
    )

    # bass_fxがbassに依存していることを明示
    set_target_properties(bass_fx PROPERTIES
        INTERFACE_LINK_LIBRARIES bass
    )

    target_link_libraries(ksmaudio PUBLIC bass bass_fx)

    # ksmaudioにRPATHを設定
    set_target_properties(ksmaudio PROPERTIES
        BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Linux/bass:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Linux/bass_fx"
    )
else()
    target_include_directories(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Windows/bass
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Windows/bass_fx
    )

    target_link_libraries(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Windows/bass/bass.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty_Windows/bass_fx/bass_fx.lib
    )
endif()

target_compile_features(ksmaudio PUBLIC cxx_std_20)
