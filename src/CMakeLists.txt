###############################################################################
#
# src/CMakeLists.txt controls building of libPC library
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
###############################################################################

# Collect dependencies configuration
if(GDAL_FOUND)
  set(LIBPC_GDAL_CPP
    gt_citation.cpp
    gt_wkt_srs.cpp
    tifvsi.cpp)
endif()

###############################################################################
# Source files specification

set(LIBPC_HEADERS_DIR ../include/libpc)

set(LIBPC_HPP
  ${LIBPC_HEADERS_DIR}/exceptions.hpp
  ${LIBPC_HEADERS_DIR}/export.hpp
  ${LIBPC_HEADERS_DIR}/version.hpp
  ${LIBPC_HEADERS_DIR}/Bounds.hpp
  ${LIBPC_HEADERS_DIR}/Color.hpp
  ${LIBPC_HEADERS_DIR}/ColorFilter.hpp
  ${LIBPC_HEADERS_DIR}/CropFilter.hpp
  ${LIBPC_HEADERS_DIR}/Dimension.hpp
  ${LIBPC_HEADERS_DIR}/FauxReader.hpp 
  ${LIBPC_HEADERS_DIR}/FauxWriter.hpp
  ${LIBPC_HEADERS_DIR}/Filter.hpp
  ${LIBPC_HEADERS_DIR}/Header.hpp
  ${LIBPC_HEADERS_DIR}/MosaicFilter.hpp
  ${LIBPC_HEADERS_DIR}/PointData.hpp  
  ${LIBPC_HEADERS_DIR}/Range.hpp
  ${LIBPC_HEADERS_DIR}/Reader.hpp
  ${LIBPC_HEADERS_DIR}/Schema.hpp
  ${LIBPC_HEADERS_DIR}/Signaller.hpp
  ${LIBPC_HEADERS_DIR}/Stage.hpp
  ${LIBPC_HEADERS_DIR}/Utils.hpp
  ${LIBPC_HEADERS_DIR}/Writer.hpp)

set(LIBPC_CPP
  version.cpp
  Bounds.cpp
  Color.cpp
  ColorFilter.cpp
  Dimension.cpp
  FauxReader.cpp
  FauxWriter.cpp
  Filter.cpp
  Header.cpp
  MosaicFilter.cpp
  PointData.cpp
  Reader.cpp
  Schema.cpp
  Signaller.cpp
  Stage.cpp
  Utils.cpp
  Writer.cpp)

# Group source files for IDE source explorers (e.g. Visual Studio)
source_group("CMake Files" FILES CMakeLists.txt)
source_group("Header Files" FILES ${LIBPC_HPP})
source_group("Source Files" FILES ${LIBPC_CPP})
if(GDAL_FOUND)
    source_group("Source Files\\gdal" FILES ${LIBPC_GDAL_CPP})
endif()

# Diable Visual C++ language extensions when building libPC library
# Need this enabled for boost to work -- hobu
#if (WIN32)
#  if (MSVC)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Za")
#  endif()
#endif()

# Standard include directory of libPC library
include_directories(../include)

###############################################################################
# Targets settings

set(LIBPC_SOURCES
  ${LIBPC_HPP}
  ${LIBPC_CPP})

# NOTE:
# This hack is required to correctly link static into shared library.
# Such practice is not recommended as not portable, instead each library,
# static and shared should be built from sources separately.
#if(UNIX)
#  add_definitions("-fPIC")
#endif()

if(WIN32)
    add_definitions("-DLIBPC_DLL_EXPORT=1")
if (NOT WITH_STATIC_LASZIP)
    add_definitions("-DLASZIP_DLL_IMPORT=1")
endif()
endif()

add_library(${LIBPC_LIB_NAME} SHARED ${LIBPC_SOURCES})

target_link_libraries(${LIBPC_LIB_NAME}
  ${LIBPC_LIB_NAME}
  ${TIFF_LIBRARY}
  ${GEOTIFF_LIBRARY}
  ${GDAL_LIBRARY}
  ${LASZIP_LIBRARY}
  ${Boost_LIBRARIES})


set_target_properties(${LIBPC_LIB_NAME}
  PROPERTIES SOVERSION "${LIBPC_LIB_SOVERSION}" )

if (APPLE)
  set_target_properties(
    ${LIBPC_LIB_NAME}
    PROPERTIES
    INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

###############################################################################
# Targets installation

install(TARGETS ${LIBPC_LIB_NAME} ${LIBPC_C_LIB_NAME}
  RUNTIME DESTINATION ${LIBPC_BIN_DIR}
  LIBRARY DESTINATION ${LIBPC_LIB_DIR}
  ARCHIVE DESTINATION ${LIBPC_LIB_DIR})

install(DIRECTORY ${LIBPC_HEADERS_DIR}
  DESTINATION ${LIBPC_INCLUDE_DIR}
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")