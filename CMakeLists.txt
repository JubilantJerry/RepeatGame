
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_FLAGS "-Wall -Werror -g -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fsanitize=address -fno-omit-frame-pointer -static-libasan")

set(PROJECT_NAME
    RepeatGameProject
)
set(LIBRARY_NAME
    RepeatGame
)
set(LIBRARY_HEADERS_DIR
    include
)
set(LIBRARY_HEADERS
    ${LIBRARY_HEADERS_DIR}/BoardGame.h
    ${LIBRARY_HEADERS_DIR}/BoardGameAI.h
    ${LIBRARY_HEADERS_DIR}/MoveList.h
    ${LIBRARY_HEADERS_DIR}/MovePicker.h
	${LIBRARY_HEADERS_DIR}/SmartVector.h
	${LIBRARY_HEADERS_DIR}/StaticVector.h
	${LIBRARY_HEADERS_DIR}/StatusUpdate.h
    ${LIBRARY_HEADERS_DIR}/TTableReplaceByDepth.h
    ${LIBRARY_HEADERS_DIR}/TTEntry.h
    ${LIBRARY_HEADERS_DIR}/types.h
)
set(LIBRARY_SOURCE_DIR
    src
)
set(LIBRARY_SOURCE
    ${LIBRARY_SOURCE_DIR}/repeat_game.cpp
)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

add_executable(${LIBRARY_NAME}
    ${LIBRARY_HEADERS}
    ${LIBRARY_SOURCE}
)

target_include_directories(${LIBRARY_NAME} PUBLIC include)

#target_include_directories(${LIBRARY_NAME} PRIVATE
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}>
#    $<INSTALL_INTERFACE:include/${LIBRARY_NAME}>
#)

#target_include_directories(${LIBRARY_NAME} PUBLIC
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#    $<INSTALL_INTERFACE:include>
#)

#add_subdirectory(test)
