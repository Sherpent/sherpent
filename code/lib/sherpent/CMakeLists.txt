idf_component_register(
        SRCS "src/led.c"
        "src/power.c"
        "src/servo.c"
        "src/sherpent.c"
        "neopixel/neopixel.c"
        INCLUDE_DIRS "include" "neopixel"
        REQUIRES driver
)

target_include_directories(${COMPONENT_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/neopixel
        )