
set(EXAMPLE helloworld)

set(RESOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/SimpleShaderVS.bin)
list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/SimpleShaderPS.bin)

add_executable(${EXAMPLE} WIN32
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${RESOURCE_FILES})

# Ensure shader library is available
add_dependencies(${EXAMPLE} CompileHLSLShaders)

set_target_properties(${EXAMPLE}
        PROPERTIES
        RESOURCE "${RESOURCE_FILES}")

target_link_libraries(${EXAMPLE} PRIVATE base)

if(TARGET Microsoft::DirectX12-Agility)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/D3D12")
    add_custom_command(TARGET ${EXAMPLE} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:${EXAMPLE}>/D3D12
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:${EXAMPLE}>/D3D12
            COMMAND_EXPAND_LISTS
    )
endif()

