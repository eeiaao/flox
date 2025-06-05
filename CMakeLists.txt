cmake_minimum_required(VERSION 3.22)
project(flox VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_DEV_SETUP "Install pre-commit hook" ON)

if(ENABLE_DEV_SETUP)
  message(STATUS "Developer setup enabled")

  set(PRECOMMIT_SRC "${CMAKE_SOURCE_DIR}/scripts/pre-commit")
  set(PRECOMMIT_DST "${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit")

  if(EXISTS "${PRECOMMIT_SRC}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${PRECOMMIT_SRC}" "${PRECOMMIT_DST}")
    execute_process(COMMAND chmod +x "${PRECOMMIT_DST}")
    message(STATUS "Installed pre-commit hook to .git/hooks")
  else()
    message(WARNING "pre-commit script not found at ${PRECOMMIT_SRC}")
  endif()
endif()

include_directories(include)

file(GLOB_RECURSE SRC_FILES
    src/**/*.cpp
)

set(FLOX ${PROJECT_NAME})
set(FLOX_SYNC ${PROJECT_NAME}_sync)

add_library(${FLOX} STATIC ${SRC_FILES})
add_library(flox::${FLOX} ALIAS ${FLOX})
target_compile_options(${FLOX} PRIVATE
  $<$<CONFIG:Release>:-O3 -march=native -flto -funroll-loops>
)

add_library(${FLOX_SYNC} STATIC ${SRC_FILES})
add_library(flox::${FLOX_SYNC} ALIAS ${FLOX_SYNC})
target_compile_definitions(${FLOX_SYNC} PRIVATE USE_SYNC_MARKET_BUS)
target_compile_options(${FLOX_SYNC} PRIVATE
  $<$<CONFIG:Release>:-O3 -march=native -flto -funroll-loops>
)

option(ENABLE_BENCHMARK "Enable benchmarks" ON)
if (ENABLE_BENCHMARK)
  add_subdirectory(benchmarks)
endif()


option(ENABLE_TESTS "Enable tests" ON)
if (ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

target_include_directories(flox PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS flox
  EXPORT floxTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT floxTargets
  FILE floxTargets.cmake
  NAMESPACE flox::
  DESTINATION lib/cmake/flox
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/floxConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/floxConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/floxConfig.cmake"
  INSTALL_DESTINATION lib/cmake/flox
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/floxConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/floxConfigVersion.cmake"
  DESTINATION lib/cmake/flox
)
