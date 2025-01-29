idf_component_register(
    SRCS "mongoose.c"
    REQUIRES "esp_timer" "esp_https_ota" "app_update"
    INCLUDE_DIRS "."
)
