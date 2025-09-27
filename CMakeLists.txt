cmake_minimum_required(VERSION 3.20)
project(Spectr)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Spectr
    main.cpp
    src/AudioTypes.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Expr.cpp
    src/interpreters/Interpreter.cpp
    src/interpreters/Types.cpp
    src/PlaybackEvents.cpp
    src/Session.cpp
    src/Synthesizer.cpp
    deps/rtmidi-6.0.0/RtMidi.cpp
)

target_include_directories(Spectr PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/interpreters
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/deps/rtmidi-6.0.0
)

if(WIN32)
    target_compile_definitions(Spectr PRIVATE __WINDOWS_MM__)
    target_link_libraries(Spectr PRIVATE winmm)
elseif(UNIX)
    target_compile_definitions(Spectr PRIVATE __LINUX_ALSA__)
    find_package(Threads REQUIRED)
    target_link_libraries(Spectr PRIVATE Threads::Threads asound)
endif()

add_custom_command(TARGET Spectr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/std
        $<TARGET_FILE_DIR:Spectr>/std
)