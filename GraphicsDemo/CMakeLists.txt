project ("GraphicsDemo")

file (GLOB headers *.h)
file (GLOB sourceFiles *.cpp)
set(ALL_FILES ${headers} ${sourceFiles})
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC 
JDEngine)

target_link_libraries(${PROJECT_NAME} PUBLIC
JDEngine)