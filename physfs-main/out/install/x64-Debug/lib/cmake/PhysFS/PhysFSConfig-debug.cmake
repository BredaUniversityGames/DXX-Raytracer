#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "PhysFS::PhysFS-static" for configuration "Debug"
set_property(TARGET PhysFS::PhysFS-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(PhysFS::PhysFS-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/physfs-static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS PhysFS::PhysFS-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_PhysFS::PhysFS-static "${_IMPORT_PREFIX}/lib/physfs-static.lib" )

# Import target "PhysFS::PhysFS" for configuration "Debug"
set_property(TARGET PhysFS::PhysFS APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(PhysFS::PhysFS PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/physfs.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/physfs.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS PhysFS::PhysFS )
list(APPEND _IMPORT_CHECK_FILES_FOR_PhysFS::PhysFS "${_IMPORT_PREFIX}/lib/physfs.lib" "${_IMPORT_PREFIX}/bin/physfs.dll" )

# Import target "PhysFS::test_physfs" for configuration "Debug"
set_property(TARGET PhysFS::test_physfs APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(PhysFS::test_physfs PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/test_physfs.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS PhysFS::test_physfs )
list(APPEND _IMPORT_CHECK_FILES_FOR_PhysFS::test_physfs "${_IMPORT_PREFIX}/bin/test_physfs.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
