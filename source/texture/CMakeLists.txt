
set(EXAMPLE texture)


add_executable(${EXAMPLE} WIN32 
    main.cpp
    Texture.cpp
)

# Ensure shader library is available
add_dependencies(${EXAMPLE} CompileHLSLShaders)

target_link_libraries(${EXAMPLE} PRIVATE base)

add_custom_command(TARGET ${EXAMPLE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/example.dds $<TARGET_FILE_DIR:${EXAMPLE}>
)

if(TARGET Microsoft::DirectX12-Agility)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/D3D12")
    add_custom_command(TARGET ${EXAMPLE} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:${EXAMPLE}>/D3D12
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:${EXAMPLE}>/D3D12
            COMMAND_EXPAND_LISTS
    )
endif()

