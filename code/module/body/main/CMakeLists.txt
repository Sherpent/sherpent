idf_component_register(SRCS "body.c" "ble.c" "storage.c"
                        INCLUDE_DIRS "." "../../../schemas" "../../../utils"
                        REQUIRES bt sherpent nvs_flash)

target_link_libraries(${COMPONENT_LIB} PRIVATE schemas)