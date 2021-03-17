set(TOOL_NAME tokenprofiler)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake"
#        "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${TOOL_NAME} main.cpp)

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES ${LLVM_TARGETS_TO_BUILD}
        asmparser core linker bitreader bitwriter irreader ipo scalaropts
        analysis target mc support)

target_link_libraries(${TOOL_NAME} ${TOOL_NAME}-impl ${REQ_LLVM_LIBRARIES})

# Platform dependencies.
if( WIN32 )
    find_library(SHLWAPI_LIBRARY shlwapi)
    target_link_libraries(${TOOL_NAME}
            ${SHLWAPI_LIBRARY})
else()
    find_package(Threads REQUIRED)
    find_package(Curses REQUIRED)
    target_link_libraries(${TOOL_NAME}
            ${CMAKE_THREAD_LIBS_INIT}
            ${CMAKE_DL_LIBS}
            ${CURSES_LIBRARIES})
endif()

set_target_properties(${TOOL_NAME}
        PROPERTIES
        LINKER_LANGUAGE CXX
        PREFIX "")

install(TARGETS ${TOOL_NAME}
        RUNTIME DESTINATION bin)

