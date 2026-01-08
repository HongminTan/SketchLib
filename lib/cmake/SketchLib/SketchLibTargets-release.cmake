#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SketchLib::SketchLib" for configuration "Release"
set_property(TARGET SketchLib::SketchLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SketchLib::SketchLib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libSketchLib.a"
  )

list(APPEND _cmake_import_check_targets SketchLib::SketchLib )
list(APPEND _cmake_import_check_files_for_SketchLib::SketchLib "${_IMPORT_PREFIX}/lib/libSketchLib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
