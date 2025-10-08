cmake_minimum_required(VERSION 3.16)
project(ksmaudio)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE KSMAUDIO_SOURCES src/*.cpp)
add_library(ksmaudio STATIC ${KSMAUDIO_SOURCES})

target_include_directories(ksmaudio PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(APPLE)
    target_include_directories(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party_macos/bass
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party_macos/bass_fx
    )
    
    add_library(bass SHARED IMPORTED)
    set_target_properties(bass PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third_party_macos/bass/libbass.dylib
    )
    
    add_library(bass_fx SHARED IMPORTED)
    set_target_properties(bass_fx PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third_party_macos/bass_fx/libbass_fx.dylib
    )
    
    target_link_libraries(ksmaudio PUBLIC bass bass_fx)
elseif(UNIX)
    target_include_directories(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party_linux/bass
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party_linux/bass_fx
    )

    add_library(bass SHARED IMPORTED)
    set_target_properties(bass PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third_party_linux/bass/libbass.so
        IMPORTED_NO_SONAME TRUE
    )

    add_library(bass_fx SHARED IMPORTED)
    set_target_properties(bass_fx PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third_party_linux/bass_fx/libbass_fx.so
        IMPORTED_NO_SONAME TRUE
    )

    # bass_fxがbassに依存していることを明示
    set_target_properties(bass_fx PROPERTIES
        INTERFACE_LINK_LIBRARIES bass
    )

    target_link_libraries(ksmaudio PUBLIC bass bass_fx)

    # ksmaudioにRPATHを設定
    set_target_properties(ksmaudio PROPERTIES
        BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party_linux/bass:${CMAKE_CURRENT_SOURCE_DIR}/third_party_linux/bass_fx"
    )
else()
    target_include_directories(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bass
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bass_fx
    )
    
    target_link_libraries(ksmaudio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bass/bass.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bass_fx/bass_fx.lib
    )
endif()

target_compile_features(ksmaudio PUBLIC cxx_std_20)
