idf_component_register(SRCS "body.c" "ble.c"
                    INCLUDE_DIRS "." "../../../schemas" "../../../utils")

target_link_libraries(${COMPONENT_LIB} PRIVATE schemas)