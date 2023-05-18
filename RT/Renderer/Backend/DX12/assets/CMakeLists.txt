cmake_minimum_required (VERSION 3.8)

#copy shaders when a change happened.
add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}
   $<TARGET_FILE_DIR:Renderer>/assets
    COMMENT "Change found in shaders or the asset folder, copying over Assets folder to output directory.")

add_dependencies(Showcase_Renderer copy_shaders)