# Based on https://www.llvm.org/docs/CMake.html#embedding-llvm-in-your-project

find_package(LLVM CONFIG)

# This example is only available if LLVM is installed.
if(LLVM_FOUND)
    include_directories(${LLVM_INCLUDE_DIRS})
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
    add_definitions(${LLVM_DEFINITIONS_LIST})
    llvm_map_components_to_libnames(llvm_libs support core irreader)

    set(SRC
        "src/main.cpp"
        "src/Token.cpp"
        "src/Lexer.cpp"
        "src/AST.cpp"
        "src/Parser.cpp"
        "src/Codegen.cpp"
    )

    add_executable("Kaleidoscope" ${SRC})
    target_include_directories("Kaleidoscope" PRIVATE "../../include/")
    target_link_libraries("Kaleidoscope" ${llvm_libs})

    target_compile_options("Kaleidoscope" PRIVATE -g)
    target_compile_options("Kaleidoscope" PRIVATE -Wall)
    target_compile_options("Kaleidoscope" PRIVATE -pedantic)

    target_compile_options("Kaleidoscope" PRIVATE -fsanitize=address)
    target_link_options("Kaleidoscope" PRIVATE -fsanitize=address)
endif()