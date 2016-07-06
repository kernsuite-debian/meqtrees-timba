# - Find wcslib
# Find the native CFITSIO includes and library
#
#  CFITSIO_INCLUDE_DIR - where to find wcslib.h, etc.
#  CFITSIO_LIBRARIES   - List of libraries when using wcslib.
#  CFITSIO_FOUND       - True if wcslib found.

IF (CFITSIO_INCLUDE_DIR)
    # Already in cache, be silent
    SET(CFITSIO_FIND_QUIETLY TRUE)
ENDIF (CFITSIO_INCLUDE_DIR)

FIND_PATH(CFITSIO_INCLUDE_DIR fitsio.h fitsio2.h PATHS /usr/include/cfitsio /usr/include/libcfitsio /usr/include/libcfitsio0 )

SET(CFITSIO_NAMES 
    cfitsio
    )
FOREACH( lib ${CFITSIO_NAMES} )
FIND_LIBRARY(CFITSIO_LIBRARY_${lib} NAMES ${lib} )
    LIST(APPEND CFITSIO_LIBRARIES ${CFITSIO_LIBRARY_${lib}})
ENDFOREACH(lib)

# handle the QUIETLY and REQUIRED arguments and set CFITSIO_FOUND to TRUE if.
# all listed variables are TRUE
INCLUDE(FindPackageHandleCompat)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CFITSIO DEFAULT_MSG CFITSIO_LIBRARIES CFITSIO_INCLUDE_DIR)

IF(NOT CFITSIO_FOUND)
    SET( CFITSIO_LIBRARIES )
ENDIF(NOT CFITSIO_FOUND)

MARK_AS_ADVANCED( CFITSIO_LIBRARIES CFITSIO_INCLUDE_DIR )

