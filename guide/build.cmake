# Build the target languages
function(BuildBook LANGUAGE SOURCE_DIR TARGET_DIR)
  set(LANGUAGE "${LANGUAGE}")

  if(NOT EXISTS "${SOURCE_DIR}/src/SUMMARY.md")
    message(WARNING "Skipping '${LANGUAGE}' – SUMMARY.md not found at ${SOURCE_DIR}")
    return()
  endif()

  if(NOT EXISTS "${SOURCE_DIR}/book.toml")
    message(WARNING "Skipping '${LANGUAGE}' – book.toml not found at ${SOURCE_DIR}")
    return()
  endif()

  message(STATUS "Building book for language: ${LANGUAGE}")
  execute_process(
    COMMAND mdbook build -d ${TARGET_DIR}
    WORKING_DIRECTORY ${SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
  )
endfunction()

# Copy the theme folder
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/theme" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/translations")

BuildBook("en" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/book")
BuildBook("zh-TW" "${CMAKE_CURRENT_SOURCE_DIR}/translations/zh-TW" "${CMAKE_CURRENT_SOURCE_DIR}/book/zh-TW")
