#include "daisy_chain.h"



const struct device *spi_dev1;
const struct device *spi_dev2;
const struct device *spi_dev3;

struct spi_config spi_cfg1;
struct spi_config spi_cfg2;
struct spi_config spi_cfg3;


K_MUTEX_DEFINE(spi_mutex);

int drv8434s_init(void) {
    printk("Initializing DRV8434S drivers...\n");
    
    // Gets the SPI bus device (not the individual driver nodes)
    const struct device *spi_bus = DEVICE_DT_GET(DT_NODELABEL(spi1));
    
    if (!device_is_ready(spi_bus)) {
        printk("ERROR: SPI1 bus not ready!\n");
        return -1;
    }
    
    // All three drivers use the same SPI bus
    spi_dev1 = spi_bus;
    spi_dev2 = spi_bus;
    spi_dev3 = spi_bus;
    
    printk("SPI bus ready\n");

    // Configure SPI settings for driver 1
    spi_cfg1.frequency = 1000000;
    spi_cfg1.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER;
    spi_cfg1.slave = 0;
    spi_cfg1.cs = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi1), cs_gpios, 0),
        .delay = 0,
    };

    // Configure SPI settings for driver 2
    spi_cfg2.frequency = 1000000;
    spi_cfg2.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER;
    spi_cfg2.slave = 1;
    spi_cfg2.cs = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi1), cs_gpios, 1),
        .delay = 0,
    };

    // Configure SPI settings for driver 3
    spi_cfg3.frequency = 1000000;
    spi_cfg3.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER;
    spi_cfg3.slave = 2;
    spi_cfg3.cs = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi1), cs_gpios, 2),
        .delay = 0,
    };

    printk("SPI configurations set\n");
    return 0;
}



// Write to a SPECIFIC driver
int drv8434s_write_reg(const struct device *spi_dev, const struct spi_config *spi_cfg,uint8_t reg, uint8_t value)
//value = bit to write, reg = register address
{

    /* DRV8434S SPI Frame (16 bits total):
            Bit 15:    Reserved (always 0)
            Bit 14:    W (Write bit) - 0=write, 1=read
            Bits 13-9: A (Address) - 5-bit register address
            Bit 8:     Reserved (always 0)
            Bits 7-0:  D (Data) - 8-bit data value
    */
    uint16_t frame = 0;
    // put 0 at bit 14 just to tell the driver that the register is being written to
    frame |= (0 << 14);          // Bit 14: W=0 for write
    // or the frame with the reg address and 1F (because it makese sure only the lower 5 bit are used) and then shift it nine bits to put it in bits 13-9
    frame |= ((reg & 0x1F) << 9); // Bits 13-9: 5-bit address
    // put the data to be transfered into the first 8 bits and and it with 0xFF because it ensures the value is only 8 bits
    frame |= (value & 0xFF);      // Bits 7-0: 8-bit data

    // Send as two bytes, MSB first
    uint8_t tx_buf[2];
    tx_buf[0] = (frame >> 8) & 0xFF;  // Upper byte & 0xFF ensures its only 8 bits
    tx_buf[1] = frame & 0xFF;          // Lower byte & 0xFF ensures its only 8 bits
    

    // zephyr spi buffer struct
    struct spi_buf tx_spi_buf = {
        .buf = tx_buf, //pointer to the data array
        .len = 2 // length of the data array (in bytes)
    };

    // zephyr set of spi buffers struct
    struct spi_buf_set tx_spi_buf_set = {
        .buffers = &tx_spi_buf, // pointer to the buffers
        .count = 1 // just 1 for now 
    };

    // spi write automatically pulls CS low based on .slave in spi_cfg
    // then transfers all bytes in the buffer
    // pull CS high when done
    int flag;
    flag = spi_write(spi_dev, spi_cfg, &tx_spi_buf_set);
    k_usleep(5);  // 5 µs to be safe

    return flag;
}


// Spi read function
int drv8434s_read_reg(const struct device *spi_dev, const struct spi_config *spi_cfg, uint8_t reg, uint8_t *value)
//value = bit to write, reg = register address
{
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];
    
    // Build 16-bit frame with read bit set
    uint16_t frame = 0;
    frame |= (1 << 14);           // Bit 14: W=1 for READ
    frame |= ((reg & 0x1F) << 9); // Bits 13-9: 5-bit address of the register being read from

    
    tx_buf[0] = (frame >> 8) & 0xFF; //lower 8 bytes (0xFF ensures its only 8)
    tx_buf[1] = frame & 0xFF;


    struct spi_buf tx_spi_buf = {
        .buf = tx_buf,
        .len = 2
    };
    struct spi_buf rx_spi_buf = {
        .buf = rx_buf,
        .len = 2
    };
    struct spi_buf_set tx_spi_buf_set = {
        .buffers = &tx_spi_buf,
        .count = 1
    };
    struct spi_buf_set rx_spi_buf_set = {
        .buffers = &rx_spi_buf,
        .count = 1
    };
    

    int flag = spi_transceive(spi_dev, spi_cfg, &tx_spi_buf_set, &rx_spi_buf_set);
    if (flag < 0) {
        printk("failed to read %d\n", flag);
        }

    k_usleep(5);  // 5 µs 


    *value = rx_buf[1];  // Register data in lower byte

    return flag;
}
