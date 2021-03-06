CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(pcre2_finder)

# parameters
OPTION(BUILD_STATIC "Build static libraries" ON)
OPTION(BUILD_SHARED "Build shared libraries" ON)
OPTION(BUILD_TOOLS "Build tools" ON)
SET(PCRE2_DIR "" CACHE PATH "Path to the PCRE2 library")

# conditions
IF(NOT BUILD_STATIC AND NOT BUILD_SHARED)
  MESSAGE(FATAL_ERROR "Cannot build with both BUILD_STATIC and BUILD_SHARED disabled")
ENDIF()

# dependancies
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
FIND_PACKAGE(PCRE2 REQUIRED)

# Doxygen
FIND_PACKAGE(Doxygen)
OPTION(BUILD_DOCUMENTATION "Create and install API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

# build parameters
SET(CMAKE_C_FLAGS "-Wall")

INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(${PCRE2_INCLUDE_DIRS})

# build definitions
SET(ALLTARGETS)
SET(LINKTYPES)
IF(BUILD_STATIC)
  LIST(APPEND LINKTYPES "STATIC")
ENDIF()
IF(BUILD_SHARED)
  LIST(APPEND LINKTYPES "SHARED")
ENDIF()

FOREACH(LINKTYPE ${LINKTYPES})
  ADD_LIBRARY(pcre2_finder_${LINKTYPE} ${LINKTYPE} lib/pcre2_finder.c lib/search_data_buffer.c)
  IF(LINKTYPE STREQUAL "SHARED")
    SET_TARGET_PROPERTIES(pcre2_finder_${LINKTYPE} PROPERTIES DEFINE_SYMBOL "BUILD_PCRE2_FINDER_DLL")
  ELSE()
    SET_TARGET_PROPERTIES(pcre2_finder_${LINKTYPE} PROPERTIES DEFINE_SYMBOL "BUILD_PCRE2_FINDER_STATIC")
  ENDIF()
  SET_TARGET_PROPERTIES(pcre2_finder_${LINKTYPE} PROPERTIES COMPILE_DEFINITIONS "${LINKTYPE}")
  SET_TARGET_PROPERTIES(pcre2_finder_${LINKTYPE} PROPERTIES OUTPUT_NAME pcre2_finder)
  TARGET_INCLUDE_DIRECTORIES(pcre2_finder_${LINKTYPE} PRIVATE lib)
  TARGET_LINK_LIBRARIES(pcre2_finder_${LINKTYPE} ${PCRE2_LIBRARIES})
  SET(ALLTARGETS ${ALLTARGETS} pcre2_finder_${LINKTYPE})

  SET(EXELINKTYPE ${LINKTYPE})
ENDFOREACH()

IF(BUILD_TOOLS)
  ADD_EXECUTABLE(pcre2_finder_count src/pcre2_finder_count.c)
  TARGET_LINK_LIBRARIES(pcre2_finder_count pcre2_finder_${EXELINKTYPE})
  LIST(APPEND ALLTARGETS pcre2_finder_count)
  ADD_EXECUTABLE(pcre2_finder_replace src/pcre2_finder_replace.c)
  TARGET_LINK_LIBRARIES(pcre2_finder_replace pcre2_finder_${EXELINKTYPE})
  LIST(APPEND ALLTARGETS pcre2_finder_replace)
ENDIF()

IF(BUILD_DOCUMENTATION)
  IF(NOT DOXYGEN_FOUND)
    MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
  ENDIF()
  ADD_CUSTOM_TARGET(doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile
    #WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
  INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/doc/man
    DESTINATION .
  )
  #INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/doc/html
  #  DESTINATION share/doc
  #)
ENDIF()

# installation specifications
INSTALL(TARGETS ${ALLTARGETS}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
INSTALL(DIRECTORY include/
  DESTINATION include 
  FILES_MATCHING PATTERN "pcre2_finder*.h"
)
