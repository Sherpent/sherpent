idf_component_register(SRCS "head.c" "ble.c" "server.c"
                    INCLUDE_DIRS "." "../../schemas")

target_link_libraries(${COMPONENT_LIB} PRIVATE schemas)