cmake_minimum_required(VERSION 3.5)

# libnmea source dirs
SET(LIBNMEA_DIR "libnmea")
SET(LIBNMEA_NMEA_DIR "${LIBNMEA_DIR}/src/nmea")
SET(LIBNMEA_PARSERS_DIR "${LIBNMEA_DIR}/src/parsers")


# List of libnmea parsers
file(GLOB PARSERS_SRCS
        RELATIVE "${LIBNMEA_DIR}"
        "${LIBNMEA_DIR}/src/parsers/*.c")
list(REMOVE_ITEM PARSERS_SRCS "${LIBNMEA_DIR}/src/parsers/parse.c")

# Number of libnmea parsers
list(LENGTH PARSERS_SRCS PARSER_CNT)

# source files list
file(GLOB SOURCES *.c)

# append libnmea static srcs
list(APPEND SOURCES "libnmea/src/nmea/nmea.c"
        "libnmea/src/nmea/parser_static.c"
        "libnmea/src/parsers/parse.c")

# register idf component
idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS ${LIBNMEA_NMEA_DIR} ${LIBNMEA_PARSERS_DIR})


# Add libnmea parsers to target sources
target_sources(${COMPONENT_LIB} PRIVATE ${PARSERS_SRCS})

# Loop through each libnmea parser and add to target sources
FOREACH (PARSER_SRC ${PARSERS_SRCS})
    # Get the name of libnmea parser
    get_filename_component(PARSER ${PARSER_SRC} NAME_WE)
    string(TOUPPER ${PARSER} PARSER_UPPERCASE)

    # Enable the libnmea parser
    target_compile_definitions(${COMPONENT_LIB} PRIVATE "ENABLE_${PARSER_UPPERCASE}=1")

    # Set compiler definitions
    # See https://github.com/jacketizer/libnmea/blob/master/CMakeLists.txt#L140-L159
    set(PARSER_COMPILE_DEFS "allocate_data=nmea_${PARSER}_allocate_data"
            "free_data=nmea_${PARSER}_free_data"
            "init=nmea_${PARSER}_init"
            "parse=nmea_${PARSER}_parse"
            "set_default=nmea_${PARSER}_set_default"
            )
    set_source_files_properties(${PARSER_SRC} PROPERTIES COMPILE_DEFINITIONS "${PARSER_COMPILE_DEFS}")
ENDFOREACH ()

target_compile_definitions(${COMPONENT_LIB} PRIVATE PARSER_COUNT=${PARSER_CNT})
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-strict-prototypes")