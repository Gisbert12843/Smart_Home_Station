#include "display/display.h"


#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "utils/helper_functions.h"


// static SemaphoreHandle_t lvgl_mux; // LVGL mutex

#define TAG "LCD"


#define BUFFER_SIZE LCD_H_RES *LCD_V_RES / 30

#define LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)
#define LCD_I80_BUS_WIDTH 8
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL

#define PIN_NUM_DATA0 ((gpio_num_t)18)
#define PIN_NUM_DATA1 ((gpio_num_t)5)
#define PIN_NUM_DATA2 ((gpio_num_t)26)
#define PIN_NUM_DATA3 ((gpio_num_t)25)
#define PIN_NUM_DATA4 ((gpio_num_t)17)
#define PIN_NUM_DATA5 ((gpio_num_t)16)
#define PIN_NUM_DATA6 ((gpio_num_t)27)
#define PIN_NUM_DATA7 ((gpio_num_t)14)

#define PIN_NUM_PCLK ((gpio_num_t)4) // WR
#define PIN_NUM_CS ((gpio_num_t)33)
#define PIN_NUM_DC ((gpio_num_t)15)
#define PIN_NUM_RST ((gpio_num_t)32)
#define PIN_NUM_BK_LIGHT ((gpio_num_t)13)

#define PIN_NUM_SDA ((gpio_num_t)21)
#define PIN_NUM_SCL ((gpio_num_t)22)
#define PIN_NUM_INT ((gpio_num_t)19)
#define PIN_NUM_CRST ((gpio_num_t)23)
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000

static esp_lcd_touch_handle_t tp; // LCD touch handle
static lv_display_t *disp;

// static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
// {
//     lv_display_t *disp_driver = (lv_display_t *)user_ctx;
//     lv_disp_flush_ready(disp_driver);
//     return false;
// }

static void lvgl_flush_cb(lv_display_t *drv, const lv_area_t *area, uint8_t *color_map)
{
    esp_lcd_panel_draw_bitmap((esp_lcd_panel_handle_t)lv_display_get_user_data(drv), area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

static void lvgl_tick_increment_cb(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}


void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev_drv);
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    assert(tp);

    /* Read data from touch controller into memory */
    esp_lcd_touch_read_data(tp);

    /* Read data from touch controller */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0)
    {
        data->point.x = -1*((touchpad_x[0] - LCD_H_RES)); //This is to turn the 0/0 point from the top/right into the top/left corner (landscape mode)
        data->point.y = touchpad_y[0];

        // ESP_LOGD("touchpad_read()", "Touchpad pressed at x: %d, y: %d", data->point.x, data->point.y);
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

esp_err_t init_i8080()
{
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_OFF_LEVEL);

    ESP_LOGI(TAG, "Initialize Intel 8080 bus");
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config =
        {
            .dc_gpio_num = PIN_NUM_DC,
            .wr_gpio_num = PIN_NUM_PCLK,
            .clk_src = LCD_CLK_SRC_DEFAULT,
            .data_gpio_nums = {
                PIN_NUM_DATA0,
                PIN_NUM_DATA1,
                PIN_NUM_DATA2,
                PIN_NUM_DATA3,
                PIN_NUM_DATA4,
                PIN_NUM_DATA5,
                PIN_NUM_DATA6,
                PIN_NUM_DATA7},
            .bus_width = LCD_I80_BUS_WIDTH,
            .max_transfer_bytes = BUFFER_SIZE * sizeof(lv_color16_t),
            .psram_trans_align = 64,
            .sram_trans_align = 32,
        };

    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 20,
        // .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = disp, // this
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .dc_levels = {
            .dc_idle_level = 1,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1},
        .flags = {.cs_active_high = 0, .reverse_color_bits = 0, .swap_color_bytes = 1, .pclk_active_neg = 0, .pclk_idle_low = 0}};

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install LCD driver of ili9488");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16};

    ESP_ERROR_CHECK(init_display_driver(io_handle, &panel_config, BUFFER_SIZE, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));

    // These are here to rotate the screen to landscape mode, dont forget to alter the LCD_H_RES and LCD_V_RES to your new POV
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL));

    helper_functions::delay(200);
    ////////////////////////////////////////////////////
    // I2C INIT
    ESP_LOGI(TAG, "INIT i2c");

    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = 0,
        .sda_io_num = PIN_NUM_SDA,
        .scl_io_num = PIN_NUM_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,

        .glitch_ignore_cnt = 7,

        .flags{.enable_internal_pullup = true},
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus));

    ////////////////////////////////////////////////////////////////////////
    helper_functions::delay(200);

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    static lv_color16_t *buf1 = (lv_color16_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(lv_color16_t), MALLOC_CAP_DMA);
    static lv_color16_t *buf2 = (lv_color16_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(lv_color16_t), MALLOC_CAP_DMA);

    ESP_LOGI(TAG, "Buffers initialized");

    disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    ESP_LOGI(TAG, "Display initialized");

    lv_display_set_buffers(disp, &buf1[0], &buf2[0], BUFFER_SIZE * sizeof(lv_color16_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "buffers set");

    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_user_data(disp, panel_handle);
    lv_display_set_driver_data(disp, panel_handle);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

    helper_functions::delay(100);

    ////////////////////////////////////////////////////
    // INIT TOUCH
    ESP_LOGI(TAG, "INIT TOUCH");

    static lv_indev_t *indev_drv_tp = lv_indev_create();
    
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = PIN_NUM_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            // if altering these values do not lead to the expected results,
            // try to alter the values read in your touchpad_read() function
            //(e.g. subtract the hor_max from the x value and multiply by -1)
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = 400000;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v2(i2c_bus, &tp_io_config, &tp_io_handle));
    ESP_ERROR_CHECK(init_touch_driver(tp_io_handle, (const esp_lcd_touch_config_t *)&tp_cfg, &tp));
    assert(tp);

    /* Register a touchpad input device */

    lv_indev_set_type(indev_drv_tp, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_drv_tp, touchpad_read);
    lv_indev_set_user_data(indev_drv_tp, tp);
    lv_indev_set_display(indev_drv_tp, disp);

    ////////////////////////////////////////////////////

    ESP_LOGI(TAG, "Install LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_increment_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
        .skip_unhandled_events = false};
    esp_timer_handle_t lvgl_tick_timer = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Display LVGL animation");
    lv_disp_get_scr_act(disp);

    return ESP_OK;
}
