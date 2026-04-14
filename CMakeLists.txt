# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("JDEngine")


set(ASSETDIR ${CMAKE_SOURCE_DIR}/Assets)
set(IMAGEDIR ${ASSETDIR}/Images)
set(MODELDIR ${ASSETDIR}/Models)
set(SHADERDIR ${ASSETDIR}/Shaders)

add_compile_definitions(
        ASSETDIR="${ASSETDIR}"
        IMAGEDIR="${IMAGEDIR}"
        MODELDIR="${MODELDIR}"
        SHADERDIR="${SHADERDIR}"
)

function (add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set (ENTRY_POINTS -entry vertMain -entry fragMain)
    add_custom_command(
           OUTPUT ${SHADERDIR}
           COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
    add_custom_command (
          OUTPUT  ${SHADERS_DIR}/slang.spv
          COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
    add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)

    message(STATUS "Shader sources: ${SHADER_SOURCES}")
endfunction()

# Include sub-projects.
add_subdirectory ("JDEngine")
add_subdirectory ("GraphicsDemo")





