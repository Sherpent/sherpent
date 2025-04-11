idf_component_register(SRCS "head.c" "ble.c" "movement.c" "effect.c"
        INCLUDE_DIRS "." "../../../schemas" "../../../utils"
        PRIV_REQUIRES bt sherpent nvs_flash)

target_link_libraries(${COMPONENT_LIB} PRIVATE schemas)