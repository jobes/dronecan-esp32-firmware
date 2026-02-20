#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "bmp3.h"
#include "bmp390.h"
#include "esp_log.h"

static spi_device_handle_t spi_dev;
static struct bmp3_dev bmp_dev;

static BMP3_INTF_RET_TYPE bus_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    spi_transaction_t t = {
        .addr = reg_addr,
        .length = len * 8,
        .tx_buffer = reg_data,
    };
    return spi_device_transmit(spi_dev, &t);
}

static BMP3_INTF_RET_TYPE bus_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    spi_transaction_t t = {
        .addr = reg_addr,
        .length = len * 8,
        .rx_buffer = reg_data,
    };
    return spi_device_transmit(spi_dev, &t);
}

static void delay_us(uint32_t period, void *intf_ptr)
{
    esp_rom_delay_us(period);
}

uint8_t bmp390_spi_init()
{
    spi_bus_config_t buscfg = {
        .miso_io_num = BMP_MISO,
        .mosi_io_num = BMP_MOSI,
        .sclk_io_num = BMP_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_SPEED_HZ,
        .mode = 0,
        .spics_io_num = BMP_CS,
        .queue_size = 7,
        .address_bits = 8,
    };

    if (spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK)
    {
        ESP_LOGE("BMP390", "Failed to initialize SPI bus");
        return ESP_FAIL;
    }

    if (spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev) != ESP_OK)
    {
        ESP_LOGE("BMP390", "Failed to add SPI device");
        return ESP_FAIL;
    }

    bmp_dev.intf = BMP3_SPI_INTF;
    bmp_dev.read = bus_spi_read;
    bmp_dev.write = bus_spi_write;
    bmp_dev.delay_us = delay_us;
    bmp_dev.intf_ptr = &spi_dev;

    if (bmp3_init(&bmp_dev) != BMP3_OK)
    {
        ESP_LOGE("BMP390", "Failed to initialize BMP3 sensor");
        return ESP_FAIL;
    }

    struct bmp3_settings settings;

    settings.press_en = BMP3_ENABLE;
    settings.temp_en = BMP3_ENABLE;
    settings.odr_filter.press_os = BMP3_OVERSAMPLING_8X;
    settings.odr_filter.temp_os = BMP3_OVERSAMPLING_2X;
    settings.odr_filter.iir_filter = BMP3_IIR_FILTER_COEFF_3;
    settings.odr_filter.odr = BMP3_ODR_25_HZ;

    if (bmp3_set_sensor_settings(BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS |
                                     BMP3_SEL_TEMP_OS | BMP3_SEL_IIR_FILTER | BMP3_SEL_ODR,
                                 &settings, &bmp_dev) != BMP3_OK)
    {
        ESP_LOGE("BMP390", "Failed to set sensor settings");
        return ESP_FAIL;
    }

    settings.op_mode = BMP3_MODE_NORMAL;
    if (bmp3_set_op_mode(&settings, &bmp_dev) != BMP3_OK)
    {
        ESP_LOGE("BMP390", "Failed to set operation mode");
        return ESP_FAIL;
    }

    return ESP_OK;
}

int8_t bmp390_get_data(struct bmp3_data *data)
{
    return bmp3_get_sensor_data(BMP3_PRESS | BMP3_TEMP, data, &bmp_dev);
}
