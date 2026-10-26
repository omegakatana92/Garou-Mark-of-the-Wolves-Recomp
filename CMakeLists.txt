cmake_minimum_required(VERSION 3.21)
project(garou VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The neogeorecomp toolkit: the in-tree copy (a submodule in a clean
# checkout), or a checkout next to this project.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp/CMakeLists.txt)
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp)
else()
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/../neogeorecomp)
endif()
set(NEOGEORECOMP_DIR ${ng_default} CACHE PATH "neogeorecomp checkout")
add_subdirectory(${NEOGEORECOMP_DIR} neogeorecomp_build)

# Your unzipped MAME "garou" set and "neogeo" system ROMs.  The build
# decrypts them into ${CMAKE_BINARY_DIR}/roms; nothing derived from them is
# committed.  Without it the player builds interpreter-only.
set(GAROU_ROM_DIR "" CACHE PATH "Directory with your garou and Neo Geo BIOS ROM files")
# Optional: entry points a previous run found at runtime (--dump-misses).
set(GAROU_ENTRIES "${CMAKE_BINARY_DIR}/entries.txt" CACHE FILEPATH "Extra entry points for the recompiler")

# Stage 0: ROM decryption (NEO-SMA program + NEO-CMC42 graphics + S ROM).
add_executable(garou_decrypt tools/decrypt.c)

# Stage 1: the generator.
add_executable(garou_recomp src/recomp_main.c src/game.c)
target_link_libraries(garou_recomp PRIVATE m68krecomp_core)
target_include_directories(garou_recomp PRIVATE src)

# Stage 2: the player, with the generated C when ROMs are available.
set(GAME_SOURCES src/main.c src/rom_select.c)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GEN_FILES 16)
if(GAROU_ROM_DIR)
    set(DEC_DIR ${CMAKE_BINARY_DIR}/roms)
    set(DEC_STAMP ${DEC_DIR}/.decrypted)
    add_custom_command(
        OUTPUT ${DEC_STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEC_DIR}
        COMMAND garou_decrypt --rom-path ${GAROU_ROM_DIR} --out ${DEC_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${DEC_STAMP}
        DEPENDS garou_decrypt
        COMMENT "Decrypting Garou ROMs (NEO-SMA + NEO-CMC42)")

    set(GEN_SOURCES ${GEN_DIR}/recomp_table.c)
    math(EXPR last "${GEN_FILES} - 1")
    foreach(i RANGE ${last})
        if(i LESS 10)
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_0${i}.c)
        else()
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_${i}.c)
        endif()
    endforeach()
    # The entry list starts empty; the `profile` target fills it from a
    # scripted play session and the next build regenerates with it.
    if(NOT EXISTS ${GAROU_ENTRIES})
        file(WRITE ${GAROU_ENTRIES} "")
    endif()
    file(MAKE_DIRECTORY ${GEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SOURCES} ${GEN_DIR}/recomp_common.h
        COMMAND garou_recomp --rom-path ${DEC_DIR} --out ${GEN_DIR} --files ${GEN_FILES}
                --entries ${GAROU_ENTRIES}
        DEPENDS garou_recomp ${GAROU_ENTRIES} ${DEC_STAMP}
        COMMENT "Recompiling Garou from ${DEC_DIR}")
    list(APPEND GAME_SOURCES ${GEN_SOURCES})
endif()

add_executable(garou ${GAME_SOURCES} src/game.c)
set_target_properties(garou PROPERTIES OUTPUT_NAME "Garou")
target_link_libraries(garou PRIVATE neogeorecomp)
target_include_directories(garou PRIVATE src ${GEN_DIR})
if(WIN32)
    # Native folder picker in rom_select.c (IFileOpenDialog).
    target_link_libraries(garou PRIVATE ole32 shell32)
endif()
if(GAROU_ROM_DIR)
    add_custom_target(profile
        COMMAND garou --headless --rom-path ${DEC_DIR} --frames 6000 --dump-misses ${GAROU_ENTRIES}
        COMMAND garou --headless --rom-path ${DEC_DIR} --frames 19000
                --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/play_long.txt --dump-misses ${GAROU_ENTRIES}
        DEPENDS garou
        COMMENT "Profiling a scripted session for runtime-only entry points")
    target_compile_definitions(garou PRIVATE GAROU_HAVE_RECOMP)
    if(MSVC)
        # Generated routines are large; /bigobj lifts the section limit.
        # /O1 avoids MSVC's superlinear optimizer blow-up on these giant
        # dispatch functions and still runs the 68000 code at full speed.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "/bigobj;/O1")
    endif()
endif()
if(MSVC)
    target_compile_definitions(garou PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(garou_recomp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(garou_decrypt PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Ship the SDL2 DLL (when the window is built) next to the executable.
if(WIN32 AND TARGET SDL2::SDL2)
    add_custom_command(TARGET garou POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:garou> $<TARGET_FILE_DIR:garou>
        COMMAND_EXPAND_LISTS)
endif()
