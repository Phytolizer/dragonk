macro(embed_file FILEPATH IDENTIFIER TARGET)
  set(filepath ${FILEPATH})
  cmake_path(GET filepath FILENAME NAME)
  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/embedded/${NAME}.c
           ${PROJECT_BINARY_DIR}/embedded/include/embedded/${NAME}.h
    DEPENDS embed ${FILEPATH}
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${PROJECT_BINARY_DIR}/embedded/include/embedded
    COMMAND
      $<TARGET_FILE:embed> ${PROJECT_SOURCE_DIR}/${FILEPATH}
      ${PROJECT_BINARY_DIR}/embedded/include/embedded/${NAME}.h
      ${PROJECT_BINARY_DIR}/embedded/${NAME}.c ${IDENTIFIER}
  )
  if(TARGET ${TARGET})
    target_sources(${TARGET} PRIVATE ${PROJECT_BINARY_DIR}/embedded/${NAME}.c)
    target_include_directories(
      ${TARGET} PRIVATE ${PROJECT_BINARY_DIR}/embedded/include
    )
  endif()
endmacro()
