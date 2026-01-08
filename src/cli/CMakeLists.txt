# Hektor CLI - Phase 3: Interactive Mode + Advanced Features
cmake_minimum_required(VERSION 3.20)

add_executable(hektor
    main.cpp
    cli.cpp
    output_formatter.cpp
    interactive_shell.cpp
    progress.cpp
    colors.cpp
    commands/db_commands.cpp
    commands/data_commands.cpp
    commands/search_commands.cpp
    commands/hybrid_commands.cpp
    commands/ingest_commands.cpp
    commands/index_commands.cpp
    commands/collection_commands.cpp
    commands/export_commands.cpp
)

target_include_directories(hektor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(hektor PRIVATE
    fmt::fmt
)

target_compile_features(hektor PRIVATE cxx_std_23)

# Install
install(TARGETS hektor DESTINATION bin)
