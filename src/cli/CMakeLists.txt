# Standalone CLI build (Phase 1)
cmake_minimum_required(VERSION 3.20)

add_executable(hektor_phase1
    main.cpp
    cli.cpp
    output_formatter.cpp
    commands/db_commands.cpp
    commands/data_commands.cpp
    commands/search_commands.cpp
)

target_include_directories(hektor_phase1 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(hektor_phase1 PRIVATE
    fmt::fmt
)

target_compile_features(hektor_phase1 PRIVATE cxx_std_23)
