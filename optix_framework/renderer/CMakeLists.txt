set(SRCS main.cpp)
set( APP_EXE renderer )
    
add_executable(${APP_EXE} ${SRCS}) 
find_package(OpenGL REQUIRED)               
target_link_libraries(${APP_EXE} framework cereal ${CUDA_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})

macro(copy_dir target folder dest)
add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${folder}
        ${dest})
endmacro(copy_dir)

if(WIN32)
file(TO_CMAKE_PATH "${OptiX_INSTALL_DIR}" OptiX_INSTALL_DIR_clean)
copy_dir(renderer "${OptiX_INSTALL_DIR_clean}/bin64" ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
copy_dir(renderer "${PROJECT_SOURCE_DIR}/externals/xercesc/dll" ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
copy_dir(renderer "${PROJECT_SOURCE_DIR}/externals/glew/bin/Release/x64" ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
copy_dir(renderer "${PROJECT_SOURCE_DIR}/externals/devil/lib/" ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
endif()

copy_dir(renderer "${SAMPLES_PTX_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/PTX_files")
copy_dir(renderer "${SAMPLES_PTX_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/PTX_files")

