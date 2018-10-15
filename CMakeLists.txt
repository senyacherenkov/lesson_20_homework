cmake_minimum_required(VERSION 3.5)

project(bulk_parallel VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

set(CMAKE_BUILD_TYPE DEBUG)

add_executable(bulkmt main.cpp observer.h observer.cpp subject.h subject.cpp utility.h utility.cpp)
add_executable(bulk_test bulk_test.cpp observer.h observer.cpp subject.h subject.cpp utility.h utility.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set_target_properties(bulkmt bulk_test PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  COMPILE_OPTIONS -Wpedantic -Wall -Wextra
)

set_target_properties(bulk_test PROPERTIES
    COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
    INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR}
)

target_link_libraries(bulkmt Threads::Threads)

target_link_libraries(bulk_test
    ${Boost_LIBRARIES}
    Threads::Threads
)

target_compile_definitions(bulk_test PRIVATE TEST_MODE)

install(TARGETS bulkmt RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT senyacherenkov@yandex.ru)

include (CPack)

enable_testing()

add_test(test_version_valid bulk_test)
