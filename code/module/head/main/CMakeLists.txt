idf_component_register(SRCS "head.c" "ble.c"
                        INCLUDE_DIRS "." "../../../schemas" "../../../utils"
                        REQUIRES driver bt sherpent)

target_link_libraries(${COMPONENT_LIB} PRIVATE schemas)