/***************************************************************************
 *
 * @file main.c
 * @brief Toggles the GPIO_8_CSI (SODIMM_222) pin of a Verdin AM62 SoM via
 * direct memory access. The pin is the GPIO0_41 pin of the AM62x TI SoCs
 *
 * @author Leonardo Costa
 * @date 2025-10-24
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// SODIMM_222: GPIO_8_CSI (SoC Ball Name: GPMC0_CSn1)
// Alternate Function: GPIO0_42

#define GPIO_BASE_ADDR 0x00600000 // GPIO0
#define BLOCK_SIZE     (4 * 1024)

// GPIO0_41: Bank 2, Register Bit 10 (Page 128)
#define GPIO_DIR01      (0x10)
#define GPIO_DIR23      (0x38)
#define GPIO_SET_DATA23 (0x40)
#define GPIO_OUT_DATA23 (0x3c)
#define GPIO_CLR_DATA23 (0x44)
#define GPIO0_42_OFFSET 10

volatile void* gpio;

int main() {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Could not open memory mapping");
        return EXIT_FAILURE;
    }

    void* gpio_map = mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        GPIO_BASE_ADDR
    );

    close(mem_fd);

    if (gpio_map == MAP_FAILED) {
        perror("Failed to create GPIO map");
        return EXIT_FAILURE;
    }

    gpio = (volatile uint32_t *) gpio_map;

    // Configure as output
    *((uint32_t*) (gpio + GPIO_DIR23)) &= ~(1 << GPIO0_42_OFFSET);

    while(1) {
        *((uint32_t*) (gpio + GPIO_SET_DATA23)) |= (1 << GPIO0_42_OFFSET);
        printf("Active\n");
        sleep(1);
        *((uint32_t*) (gpio + GPIO_CLR_DATA23)) |= (1 << GPIO0_42_OFFSET);
        printf("Inactive\n");
        sleep(1);
    }

    return 0;
}
