cmake_minimum_required(VERSION 3.16)
project(sd_phone_input LANGUAGES CXX)

if(ANDROID)
    # This shared library is loaded by PhoneNative and linked by the future
    # phone engine target, so both use the same process-wide input state.
    add_library(sd_phone_input SHARED
        phone_input.cpp
        phone_engine_adapter.cpp
        phone_input_jni.cpp
    )
    set_target_properties(sd_phone_input PROPERTIES OUTPUT_NAME sdphoneinput)
    set(SD_PHONE_INPUT_TESTS_DEFAULT OFF)
else()
    add_library(sd_phone_input STATIC
        phone_input.cpp
        phone_engine_adapter.cpp
    )
    set(SD_PHONE_INPUT_TESTS_DEFAULT ON)
endif()

target_include_directories(sd_phone_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(sd_phone_input PUBLIC cxx_std_17)
target_compile_options(sd_phone_input PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(sd_phone_input PROPERTIES POSITION_INDEPENDENT_CODE ON)

option(SD_PHONE_INPUT_BUILD_TESTS "Build platform-neutral phone input tests" ${SD_PHONE_INPUT_TESTS_DEFAULT})
if(SD_PHONE_INPUT_BUILD_TESTS)
    enable_testing()
    add_executable(sd_phone_input_test tests/phone_input_test.cpp)
    target_link_libraries(sd_phone_input_test PRIVATE sd_phone_input)
    target_compile_options(sd_phone_input_test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME sd_phone_input_test COMMAND sd_phone_input_test)
endif()
