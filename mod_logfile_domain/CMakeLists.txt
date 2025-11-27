cmake_minimum_required(VERSION 3.10)
project(mod_logfile_domain C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find FreeSWITCH
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREESWITCH REQUIRED freeswitch)

# Add include directories
include_directories(${FREESWITCH_INCLUDE_DIRS})

# Create the module library
add_library(mod_logfile_domain SHARED mod_logfile_domain.c)

# Link against FreeSWITCH and pthread
target_link_libraries(mod_logfile_domain ${FREESWITCH_LIBRARIES} pthread)

# Set output directory
set_target_properties(mod_logfile_domain PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Installation target
install(TARGETS mod_logfile_domain
        LIBRARY DESTINATION lib/freeswitch/mod)

# Installation for config
install(FILES modules.conf.xml
        DESTINATION etc/freeswitch/autoload_configs
        RENAME mod_logfile_domain.conf.xml)

# Compiler flags
if(UNIX AND NOT APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Wextra")
endif()
