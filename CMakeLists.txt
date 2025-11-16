idf_component_register(
    SRCS
        "src/bmi270_spi.c"
        "src/bmi270_config_file.c"
        "src/bmi270_init.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver      # SPI Master, GPIO
        esp_timer   # Timing functions
)
