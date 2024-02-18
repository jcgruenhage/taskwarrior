cmake_minimum_required (VERSION 3.22)

# Add a CMake option to control whether the `bundled` feature should be disabled.
OPTION(DISABLE_BUNDLED "Disable the `bundled` feature in taskchampion" OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/taskchampion-cpp/corrosion)

# Import taskchampion-lib as a CMake library. This implements the Rust side of
# the cxxbridge, and depends on the `taskchampion` crate.
if(DISABLE_BUNDLED)
    message("-- Disabling `bundled` feature for taskchampion")
    corrosion_import_crate(
      MANIFEST_PATH "${CMAKE_SOURCE_DIR}/Cargo.toml"
      LOCKED
      CRATES "taskchampion-lib"
      NO_DEFAULT_FEATURES
      FEATURES "sync"
    )
else()
  corrosion_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/Cargo.toml"
    LOCKED
    CRATES "taskchampion-lib"
  )
endif()

# Set up `taskchampion-cpp`, the C++ side of the bridge.
corrosion_add_cxxbridge(taskchampion-cpp
  CRATE taskchampion_lib
  FILES lib.rs
)
