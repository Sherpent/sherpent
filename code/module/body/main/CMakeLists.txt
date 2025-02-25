idf_component_register(SRCS "body.c" "ble.c"
                        INCLUDE_DIRS "." "../../../schemas" "../../../utils"
                        REQUIRES driver bt sherpent nvs_flash)

target_link_libraries(${COMPONENT_LIB} PRIVATE schemas)