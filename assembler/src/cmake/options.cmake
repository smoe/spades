# -*- cmake -*-

# Default configuration
set(SPADES_DEFAULT_BUILD_TYPE "RelWithDebInfo" CACHE STRING "SPAdes default build type")
if (NOT CMAKE_BUILD_TYPE)
  message("Setting default build configuration: ${SPADES_DEFAULT_BUILD_TYPE}")
  set(CMAKE_BUILD_TYPE "${SPADES_DEFAULT_BUILD_TYPE}" CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo."
      FORCE)
endif()

# Define option for static / dynamic build.
option(SPADES_STATIC_BUILD "Link SPAdes statically" OFF)
if (SPADES_STATIC_BUILD)
  # it'll make cmake to find libraries archives, not dynamic link
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a) 

  if (NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  endif()
  add_definitions(-static)

  set(Boost_USE_STATIC_LIBS        ON)
  set(Boost_USE_STATIC_RUNTIME     ON)
endif()

# Define minimum and maximum K
set(SPADES_MIN_K 1 CACHE INTEGER "Minimum k-mer length")
set(SPADES_MAX_K 127 CACHE INTEGER "Maximum k-mer length")
configure_file("${SPADES_MAIN_INCLUDE_DIR}/k_range.hpp.in"
               "${SPADES_BUILT_INCLUDE_DIR}/k_range.hpp")

# Define boost root
set(SPADES_BOOST_ROOT "" CACHE PATH "Boost root used to build SPAdes")

# Various internal stuff
option(SPADES_BUILD_INTERNAL "Build internal projects" OFF)
option(SPADES_USE_TCMALLOC "Link spades with TCMalloc" OFF)
if (SPADES_USE_TCMALLOC)
  find_package(GooglePerfTools REQUIRED)

  if (GOOGLE_PERFTOOLS_ROOT)
    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  endif()
endif()
