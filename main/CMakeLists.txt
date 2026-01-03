idf_component_register(
    SRCS "env_sensor.c" "i2c_init.c"
    INCLUDE_DIRS "."
    REQUIRES aht20 bmp280
    PRIV_REQUIRES app_update
)
