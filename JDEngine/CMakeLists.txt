# CMakeList.txt : CMake project for JDEngine, include source and define
# project specific logic here.
#

project ("JDEngine")

# Add source to this project's executable.


file (GLOB headers *.h)
file (GLOB sourceFiles *.cpp)
set(ALL_FILES ${headers} ${sourceFiles})
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})



if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET JDEngine PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

include(FetchContent)

FetchContent_Declare(
    fetch_GLFW
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(fetch_GLFW)

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader
    GIT_TAG v1.0.6
)
FetchContent_MakeAvailable(tinyobjloader)

find_package(Vulkan REQUIRED)
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

target_compile_definitions(Vulkan::Vulkan INTERFACE
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_link_libraries(${PROJECT_NAME}
  Vulkan::Vulkan
  glfw
  tinyobjloader
)

file(GLOB SHADER_SLANG_SOURCES "${SHADERDIR}/*.slang")

message(STATUS "Shader directory: ${SHADERDIR}")
message(STATUS "Shader files: ${SHADER_SLANG_SOURCES}")

if(NOT SHADER_SLANG_SOURCES)
  message(WARNING "No .slang files found in ${SHADERDIR}. Re-run CMake after adding files; file(GLOB) is evaluated at configure time.")
endif()

add_slang_shader_target(Shader_files SOURCES ${SHADER_SLANG_SOURCES})
add_dependencies(${PROJECT_NAME} Shader_files)