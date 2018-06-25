# ======================================================================
#  artdaq_mfextensions main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/artdaq/ups/setup_for_development
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -Dqualifier:STRING=$CETPKG_QUAL
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package
# ======================================================================

# use cmake 2.8 or later
# cmake_minimum_required (VERSION 2.8)
# cmake 2.8.11 or later is required for Qt
cmake_minimum_required(VERSION 2.8.11)

project(artdaq-mfextensions)
# cetbuildtools contains our cmake modules
SET ( CETBUILDTOOLS_VERSION $ENV{CETBUILDTOOLS_VERSION} )
IF (NOT CETBUILDTOOLS_VERSION)
    MESSAGE (FATAL_ERROR "ERROR: setup cetbuildtools to get the cmake modules")
ENDIF()

set(CMAKE_MODULE_PATH #$ENV{ART_DIR}/Modules
		      $ENV{CETBUILDTOOLS_DIR}/Modules
		      ${CMAKE_MODULE_PATH})

include(CetCMakeEnv)
cet_cmake_env()
check_ups_version(gcc $ENV{GCC_VERSION} v7_2_0 PRODUCT_OLDER_VAR STDCXX14)
if(STDCXX14 AND NOT DEFINED ENV{CLANG_VERSION}) # CLANG uses CXX_STANDARD 17
set(CMAKE_CXX_STANDARD 14)
else()
set(CMAKE_CXX_STANDARD 17)
endif()

cet_set_compiler_flags(DIAGS VIGILANT
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic
)

string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
if( ${BTYPE_UC} MATCHES "DEBUG" )
  #cet_add_compiler_flags(-fsanitize=thread -ltsan -Wno-unused-command-line-argument)
  cet_add_compiler_flags(-fsanitize=address -lasan -Wno-unused-command-line-argument)
endif()

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( messagefacility v1_14_00 )
find_ups_product( cetbuildtools   v4_01_03 )
find_ups_product( cetlib          v0_4_11 )
find_ups_product( TRACE           v3_06_00 )


string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" MESSAGEFACILITY_VERSION_LIST "$ENV{MESSAGEFACILITY_VERSION}")
list(GET MESSAGEFACILITY_VERSION_LIST 0 MESSAGEFACILITY_MAJOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 1 MESSAGEFACILITY_MINOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 2 MESSAGEFACILITY_PATCH_VERSION)
message("-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION}")
add_definitions(-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION})


# as of art v2_11_00, messagefacility has a single library
check_ups_version(messagefacility $ENV{MESSAGEFACILITY_VERSION} v2_02_00 PRODUCT_MATCHES_VAR ONE_MF_LIBRARY)
if(ONE_MF_LIBRARY)
    set( MESSAGEFACILITY_LIBS MF_MessageLogger)
else()
    set( MESSAGEFACILITY_LIBS MF_MessageLogger MF_Utilities)
endif()

# Qt
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC_RELAXED_MODE ON)
find_package(Qt5Widgets)
find_package(Qt5Core)

# Debug streamer.
string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC)
if(BTYPE_UC STREQUAL DEBUG)
  add_definitions(-DDEBUGME)
endif()

# macros for artdaq_dictionary and simple_plugin
include(BasicPlugin)

# Configuration Files
add_subdirectory(fcl)

# API Documentation
add_subdirectory(doc)

# MFExtensions Library
add_subdirectory(mfextensions)

# tools - utility scripts
add_subdirectory(tools)

# ups - table and config files
add_subdirectory(ups)

# packaging utility
include(UseCPack)
