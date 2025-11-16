idf_component_register(
    SRCS
        "src/bmi270_spi.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver      # SPI Master, GPIO
        esp_timer   # Timing functions
)
