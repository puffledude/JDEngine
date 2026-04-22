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
set(GLTFDIR ${MODELDIR}/GLTF)
set(SHADERDIR ${ASSETDIR}/Shaders)

add_compile_definitions(
        ASSETDIR="${ASSETDIR}"
        IMAGEDIR="${IMAGEDIR}"
        MODELDIR="${MODELDIR}"
        GLTFDIR="${GLTFDIR}"
        SHADERDIR="${SHADERDIR}"
)

function (add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(ENTRY_POINTS -entry vertMain -entry fragMain)
    set(SPV_OUTPUTS "")

    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCES)
        # Extract the source file name (e.g. "gbuffer.slang")
        get_filename_component(SHADER_NAME "${SHADER_SOURCE}" NAME)
        
        # Define the output file path (e.g. "gbuffer.slang.spv")
        set(OUTPUT_SPV "${SHADERDIR}/${SHADER_NAME}.spv")

        # Create a build command specifically for this single shader
        add_custom_command(
            OUTPUT ${OUTPUT_SPV}
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCE} -o ${OUTPUT_SPV} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS}
            WORKING_DIRECTORY ${SHADERDIR}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling Slang Shader: ${SHADER_NAME}"
            VERBATIM
        )
        
        # Keep track of outputs
        list(APPEND SPV_OUTPUTS ${OUTPUT_SPV})
    endforeach()

    # The target now depends on all individually generated .spv files
    add_custom_target(${TARGET} DEPENDS ${SPV_OUTPUTS})

    message(STATUS "Shader sources: ${SHADER_SOURCES}")
endfunction()

# Include sub-projects.
add_subdirectory ("JDEngine")
add_subdirectory ("GraphicsDemo")





