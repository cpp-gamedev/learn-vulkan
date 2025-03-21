project(learn-vk-ext)

# extract src.zip
file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")

# add GLFW to build tree
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
message(STATUS "[glfw]")
add_subdirectory(src/glfw)
add_library(glfw::glfw ALIAS glfw)

# add GLM to build tree
set(GLM_ENABLE_CXX_20 ON)
message(STATUS "[glm]")
add_subdirectory(src/glm)
target_compile_definitions(glm PUBLIC
  GLM_FORCE_XYZW_ONLY
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
  GLM_ENABLE_EXPERIMENTAL
  GLM_EXT_INCLUDED
)

# add Vulkan-Headers to build tree
message(STATUS "[Vulkan-Headers]")
add_subdirectory(src/Vulkan-Headers)

# declare ext library target
add_library(${PROJECT_NAME} INTERFACE)
add_library(learn-vk::ext ALIAS ${PROJECT_NAME})

# link to all dependencies
target_link_libraries(${PROJECT_NAME} INTERFACE
  glfw::glfw
  glm::glm
  Vulkan::Headers
)

# setup preprocessor defines
target_compile_definitions(${PROJECT_NAME} INTERFACE
  GLFW_INCLUDE_VULKAN # enable GLFW's Vulkan API
  VK_NO_PROTOTYPES # Dynamically load Vulkan at runtime
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  # link to dynamic loader
  target_link_libraries(${PROJECT_NAME} INTERFACE dl)
endif()
