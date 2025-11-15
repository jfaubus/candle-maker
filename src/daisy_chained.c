#include "daisy_chain.h"

int drv8434s_chain_init(struct drv8434s_chain *chain)
{
    // Get SPI device
    chain->spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
    if (!device_is_ready(chain->spi_dev)) {
        printk("SPI device not ready\n");
        return -1;  // error code
    }
    
    // Configure SPI - Mode 3 (CPOL=1, CPHA=1)
    //(chain*).spi_cfg
    chain->spi_cfg.frequency = 1000000;  // Start with 1 MHz
    chain->spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
                               SPI_MODE_CPOL | SPI_MODE_CPHA;
    chain->spi_cfg.slave = 0;
    chain->spi_cfg.cs = NULL;
    
    printk("DRV8434S chain initialized\n");
    return 0;
}

// this function works for writing to 1 register on 1motor driver
int drv8434s_write_register(struct drv8434s_chain *chain, uint8_t device_num, 
                             uint8_t reg_addr, uint8_t data)
{
    if (device_num < 1 || device_num > NUM_DEVICES) {
        printk("Invalid device: %d\n", device_num);
        return -1;  // Simple error code
    }
    
    // frame size is 8 bytes for 3 device WILL HAvE TO CHANGE**************************
    uint8_t tx_buf[FRAME_SIZE] = {0};
    uint8_t rx_buf[FRAME_SIZE] = {0};
    
    // Header- both headers have to start with 10 (according to datasheet) and the last 6 bits for num of devices
    tx_buf[0] = HDR1_BASE | NUM_DEVICES;  // 0x83 for 3 devices
    // second bit for header 2: global Fault bit = 1, 0 = dont care
    tx_buf[1] = HDR2_BASE;                 // 0x80
    
    // Address for target device
    int addr_idx = 2 + (NUM_DEVICES - device_num);
    //AND with 0x1F to make sure only the lower five bits are used
    tx_buf[addr_idx] = reg_addr & 0x1F;  // Bit 6: 0 for write, 1 for read
    
    // Data for target device
    // 5 WILL CHNAGE IF YOU HAVE MORE DEVICES****************************
    //          *because data bytes start at index 5 when theres three motor drivers
    int data_idx = 5 + (NUM_DEVICES - device_num);
    tx_buf[data_idx] = data;
    
    // Setup SPI buffers for zephyr
    struct spi_buf tx_spi_buf = { .buf = tx_buf, .len = FRAME_SIZE };
    struct spi_buf_set tx_set = { .buffers = &tx_spi_buf, .count = 1 };
    struct spi_buf rx_spi_buf = { .buf = rx_buf, .len = FRAME_SIZE };
    struct spi_buf_set rx_set = { .buffers = &rx_spi_buf, .count = 1 };
    
    int ret = spi_transceive(chain->spi_dev, &chain->spi_cfg, &tx_set, &rx_set);
    
    if (ret < 0) {
        printk("SPI write failed: %d\n", ret);
    } else {
        printk("Wrote 0x%02X to dev %d reg 0x%02X\n", data, device_num, reg_addr);
    }
    
    k_busy_wait(1);  // 500ns min between frames
    
    return ret;
}

int drv8434s_read_register(struct drv8434s_chain *chain, uint8_t device_num, 
                            uint8_t reg_addr, uint8_t *data)
{
    if (device_num < 1 || device_num > NUM_DEVICES || data == NULL) {
        return -1;  // Simple error code
    }
    
    uint8_t tx_buf[FRAME_SIZE] = {0};
    uint8_t rx_buf[FRAME_SIZE] = {0};
    int ret;
    
    // Transaction 1: Request the read
    tx_buf[0] = HDR1_BASE | NUM_DEVICES;
    tx_buf[1] = HDR2_BASE;
    
    int addr_idx = 2 + (NUM_DEVICES - device_num);
    tx_buf[addr_idx] = 0x40 | (reg_addr & 0x1F);  // Read: W0=1
    
    struct spi_buf tx_spi_buf = { .buf = tx_buf, .len = FRAME_SIZE };
    struct spi_buf_set tx_set = { .buffers = &tx_spi_buf, .count = 1 };
    struct spi_buf rx_spi_buf = { .buf = rx_buf, .len = FRAME_SIZE };
    struct spi_buf_set rx_set = { .buffers = &rx_spi_buf, .count = 1 };
    
    // Send read request
    ret = spi_transceive(chain->spi_dev, &chain->spi_cfg, &tx_set, &rx_set);
    if (ret < 0) {
        printk("SPI read request failed: %d\n", ret);
        return ret;
    }
    
    k_busy_wait(1);
    
    // Transaction 2: Clock out the data
    memset(tx_buf, 0, FRAME_SIZE);
    tx_buf[0] = HDR1_BASE | NUM_DEVICES;
    tx_buf[1] = HDR2_BASE;
    
    ret = spi_transceive(chain->spi_dev, &chain->spi_cfg, &tx_set, &rx_set);
    
    if (ret == 0) {
        int report_idx = 5 + (NUM_DEVICES - device_num);
        *data = rx_buf[report_idx];
        printk("Read 0x%02X from dev %d reg 0x%02X\n", *data, device_num, reg_addr);
    } else {
        printk("SPI read completion failed: %d\n", ret);
    }
    
    k_busy_wait(1);
    
    return ret;
}