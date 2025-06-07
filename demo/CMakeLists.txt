add_executable(flox_demo
    src/main.cpp
    src/demo_connector.cpp
    src/demo_strategy.cpp
    src/demo_builder.cpp
)

target_include_directories(flox_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                            ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(flox_demo PRIVATE ${FLOX})
