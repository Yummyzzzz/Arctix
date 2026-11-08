#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ArctixC::ArctixC" for configuration "MinSizeRel"
set_property(TARGET ArctixC::ArctixC APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(ArctixC::ArctixC PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/ArctixC.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/ArctixC.dll"
  )

list(APPEND _cmake_import_check_targets ArctixC::ArctixC )
list(APPEND _cmake_import_check_files_for_ArctixC::ArctixC "${_IMPORT_PREFIX}/lib/ArctixC.lib" "${_IMPORT_PREFIX}/bin/ArctixC.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
