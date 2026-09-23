cmake_minimum_required(VERSION 3.22)
project(EmbeddedCompatibilityTests LANGUAGES CXX)
enable_testing()
set(UE4SS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
include("${UE4SS_ROOT}/cmake/EmbeddedCompatibility.cmake")
add_executable(compatibility_tests
    catalog.cpp
    "${UE4SS_ROOT}/UE4SS/src/Compatibility/Catalog.cpp"
    "${UE4SS_ROOT}/UE4SS/src/Compatibility/Fingerprint.cpp"
    "${UE4SS_COMPATIBILITY_SOURCE}")
target_include_directories(compatibility_tests PRIVATE "${UE4SS_ROOT}/UE4SS/include")
target_compile_features(compatibility_tests PRIVATE cxx_std_20)
target_compile_options(compatibility_tests PRIVATE /W4 /WX /EHsc)
target_link_libraries(compatibility_tests PRIVATE bcrypt)
add_test(NAME compatibility_catalog COMMAND compatibility_tests "${UE4SS_COMPATIBILITY_ROOT}")
add_test(NAME compatibility_generator COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/test_generator.py")
