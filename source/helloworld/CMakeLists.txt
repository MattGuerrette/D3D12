
set(EXAMPLE helloworld)

set(RESOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/SimpleShaderVS.bin)
list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/SimpleShaderPS.bin)

add_executable(${EXAMPLE} WIN32
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${RESOURCE_FILES})

# Ensure shader library is available
add_dependencies(${EXAMPLE} CompileHLSLShaders)

target_link_libraries(${EXAMPLE} base)
set_target_properties(${EXAMPLE}
        PROPERTIES
        RESOURCE "${RESOURCE_FILES}")

