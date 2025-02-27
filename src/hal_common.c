#include <stdbool.h>
#include <string.h>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "chipid.h"
#include "hal_common.h"

sensor_addr_t *possible_i2c_addrs;

int (*open_i2c_sensor_fd)();
int (*open_spi_sensor_fd)();
bool (*close_sensor_fd)(int fd);
read_register_t i2c_read_register;
read_register_t spi_read_register;
int (*i2c_write_register)(int fd, unsigned char i2c_addr, unsigned int reg_addr,
                          unsigned int reg_width, unsigned int data,
                          unsigned int data_width);
int (*i2c_change_addr)(int fd, unsigned char addr);
float (*hal_temperature)();
void (*hal_cleanup)();

void (*hal_detect_ethernet)(cJSON *root);

int universal_open_sensor_fd(const char *dev_name) {
    int fd;

    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
#ifndef NDEBUG
        printf("Open %s error!\n", dev_name);
#endif
        return -1;
    }

    return fd;
}

bool universal_close_sensor_fd(int fd) {
    if (fd < 0)
        return false;

    return close(fd) == 0;
}

// Set I2C slave address,
// actually do nothing
int dummy_sensor_i2c_change_addr(int fd, unsigned char addr) { return 0; }

// Universal I2C code
int universal_sensor_i2c_change_addr(int fd, unsigned char addr) {
    if (ioctl(fd, I2C_SLAVE_FORCE, addr >> 1) < 0) {
        return -1;
    }
    return 0;
}

int universal_sensor_write_register(int fd, unsigned char i2c_addr,
                                    unsigned int reg_addr,
                                    unsigned int reg_width, unsigned int data,
                                    unsigned int data_width) {
    char buf[2];

    if (reg_width == 2) {
        buf[0] = (reg_addr >> 8) & 0xff;
        buf[1] = reg_addr & 0xff;
    } else {
        buf[0] = reg_addr & 0xff;
    }

    if (write(fd, buf, data_width) != data_width) {
        return -1;
    }
    return 0;
}

int universal_sensor_read_register(int fd, unsigned char i2c_addr,
                                   unsigned int reg_addr,
                                   unsigned int reg_width,
                                   unsigned int data_width) {
    char recvbuf[4];
    unsigned int data;

    if (reg_width == 2) {
        recvbuf[0] = (reg_addr >> 8) & 0xff;
        recvbuf[1] = reg_addr & 0xff;
    } else {
        recvbuf[0] = reg_addr & 0xff;
    }

    int data_size = reg_width * sizeof(unsigned char);
    if (write(fd, recvbuf, data_size) != data_size) {
        return -1;
    }

    data_size = data_width * sizeof(unsigned char);
    if (read(fd, recvbuf, data_size) != data_size) {
        return -1;
    }

    if (data_width == 2) {
        data = recvbuf[0] | (recvbuf[1] << 8);
    } else
        data = recvbuf[0];

    return data;
}

void setup_hal_drivers() {
    if (!strcmp(VENDOR_HISI, chip_manufacturer))
        setup_hal_hisi();
    else if (!strcmp(VENDOR_GOKE, chip_manufacturer) && *chip_name == '7') {
        setup_hal_hisi();
        strcpy(short_manufacturer, "GK");
    } else if (!strcmp(VENDOR_XM, chip_manufacturer))
        setup_hal_xm();
    else if (!strcmp(VENDOR_SSTAR, chip_manufacturer))
        setup_hal_sstar();
    else if (!strcmp(VENDOR_NOVATEK, chip_manufacturer))
        setup_hal_novatek();
    else if (!strcmp(VENDOR_GM, chip_manufacturer))
        setup_hal_gm();
    else if (!strcmp(VENDOR_FH, chip_manufacturer))
        setup_hal_fh();
}

typedef struct meminfo {
    unsigned long MemTotal;
} meminfo_t;
meminfo_t mem;

static void parse_meminfo(struct meminfo *g) {
    char buf[60];
    FILE *fp;
    int seen_cached_and_available_and_reclaimable;

    fp = fopen("/proc/meminfo", "r");
    g->MemTotal = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        if (sscanf(buf, "MemTotal: %lu %*s\n", &g->MemTotal) == 1)
            break;
    }
    fclose(fp);
}

unsigned long kernel_mem() {
    if (!mem.MemTotal)
        parse_meminfo(&mem);
    return mem.MemTotal;
}

uint32_t rounded_num(uint32_t n) {
    int i;
    for (i = 0; n; i++) {
        n /= 2;
    }
    return 1 << i;
}

void hal_ram(unsigned long *media_mem, uint32_t *total_mem) {
    if (!strcmp(VENDOR_HISI, chip_manufacturer))
        *total_mem = hisi_totalmem(media_mem);
    else if (!strcmp(VENDOR_GOKE, chip_manufacturer) && *chip_name == '7')
        *total_mem = hisi_totalmem(media_mem);
    else if (!strcmp(VENDOR_XM, chip_manufacturer))
        *total_mem = xm_totalmem(media_mem);
    else if (!strcmp(VENDOR_SSTAR, chip_manufacturer))
        *total_mem = sstar_totalmem(media_mem);
    else if (!strcmp(VENDOR_NOVATEK, chip_manufacturer))
        *total_mem = novatek_totalmem(media_mem);
    else if (!strcmp(VENDOR_GM, chip_manufacturer))
        *total_mem = gm_totalmem(media_mem);
    else if (!strcmp(VENDOR_FH, chip_manufacturer))
        *total_mem = fh_totalmem(media_mem);

    if (!*total_mem)
        *total_mem = kernel_mem();
}
