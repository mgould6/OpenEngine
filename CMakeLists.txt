cmake_minimum_required(VERSION 3.10)

project(OpenEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
elseif(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/mjgou/source/repos/OpenEngine/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)

add_executable(OpenEngine 
    main.cpp 
    Shader.cpp 
    Mesh.cpp 
    Model.cpp 
    Engine.cpp 
    RenderUtils.cpp 
    CascadedShadowMap.cpp 
    stb_image.cpp 
    GuiManager.cpp 
    PhysicsManager.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(OpenEngine PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${GLAD_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
    ${IMGUI_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_SOURCE_DIR}/src
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(OpenEngine PRIVATE
        ${OPENGL_LIBRARIES}
        glfw
        glad::glad
        assimp::assimpd
        imgui::imgui
        Bullet::BulletDynamics
        Bullet::BulletCollision
        Bullet::LinearMath
    )
else()
    target_link_libraries(OpenEngine PRIVATE
        ${OPENGL_LIBRARIES}
        glfw
        glad::glad
        assimp::assimp
        imgui::imgui
        Bullet::BulletDynamics
        Bullet::BulletCollision
        Bullet::LinearMath
    )
endif()

add_custom_command(TARGET OpenEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:assimp::assimp>/$<TARGET_FILE_NAME:assimp::assimp>"
    $<TARGET_FILE_DIR:OpenEngine>
)
