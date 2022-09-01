find_program(GPerf_EXECUTABLE NAMES gperf)

if(GPerf_EXECUTABLE)
  execute_process(
    COMMAND ${GPerf_EXECUTABLE} -v
    OUTPUT_VARIABLE _version_string
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(_version_string MATCHES "^GNU gperf ([-0-9\\.]+)")
    set(GPerf_VERSION "${CMAKE_MATCH_1}")
  endif()
  unset(_version_string)
else()
  set(GPerf_VERSION)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  GPerf
  FOUND_VAR GPerf_FOUND
  REQUIRED_VARS GPerf_EXECUTABLE
  VERSION_VAR GPerf_VERSION
)

mark_as_advanced(GPerf_EXECUTABLE)

if(GPerf_FOUND)
  if(NOT TARGET GPerf::GPerf)
    add_library(GPerf::GPerf UNKNOWN IMPORTED)
    set_target_properties(
      GPerf::GPerf PROPERTIES IMPORTED_LOCATION "${GPerf_EXECUTABLE}"
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  GPerf PROPERTIES
  URL "https://www.gnu.org/software/gperf/"
  DESCRIPTION "Perfect hash function generator"
)

include(CMakeParseArguments)

function(gperf_generate input_file output_file _target_or_sources_var)
  set(oneValueArgs GENERATION_FLAGS)
  cmake_parse_arguments(ARGS "" "${oneValueArgs}" "" ${ARGN})

  if(ARGS_UNPARSED_ARGUMENTS)
    message(
      FATAL_ERROR
        "Unknown keywords given to gperf_generate(): \"${ARGS_UNPARSED_ARGUMENTS}\""
    )
  endif()
  if(TARGET "${_target_or_sources_var}")
    get_target_property(
      aliased_target "${_target_or_sources_var}" ALIASED_TARGET
    )
    if(aliased_target)
      message(FATAL_ERROR "gperf_generate() does not work on aliased targets")
    endif()
  endif()

  get_filename_component(_infile "${input_file}" ABSOLUTE)
  set(_extraopts "${ARGS_GENERATION_FLAGS}")
  separate_arguments(_extraopts)
  cmake_path(REMOVE_FILENAME output_file OUTPUT_VARIABLE _outfile_dir)
  add_custom_command(
    OUTPUT "${output_file}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_outfile_dir}"
    COMMAND ${GPerf_EXECUTABLE} ${_extraopts} --output-file=${output_file}
            "${_infile}"
    DEPENDS "${_infile}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
  )

  if(TARGET "${_target_or_sources_var}")
    target_sources("${_target_or_sources_var}" PRIVATE "${output_file}")
  else()
    set("${_target_or_sources_var}"
        "${${_target_or_sources_var}}" "${output_file}"
        PARENT_SCOPE
    )
  endif()
endfunction()
