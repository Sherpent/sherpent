idf_component_register(SRCS "head.c" "ble.h"
                    INCLUDE_DIRS "." "../../../schemas" "../../../utils")

target_link_libraries(${COMPONENT_LIB} PRIVATE schemas)