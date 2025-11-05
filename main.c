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

#include <bits/time.h>
#include <bits/types/struct_sched_param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

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


struct period_info {
    struct timespec next_period;
    long period_ns;
};

static void periodic_task_init(struct period_info* pinfo, long nsec) {
    pinfo->period_ns = nsec;
    clock_gettime(CLOCK_MONOTONIC, &(pinfo->next_period));
}

static void inc_period(struct period_info* pinfo) {
    pinfo->next_period.tv_nsec += pinfo->period_ns;

    while (pinfo->next_period.tv_nsec >= 1000000000) {
        pinfo->next_period.tv_sec++;
        pinfo->next_period.tv_nsec -= 1000000000;
    }
}

static void wait_rest_of_period(struct period_info* pinfo) {
    inc_period(pinfo);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}

void* toggle_gpio_task(void* gpio) {
    volatile uint32_t* gpio_set_data_reg = (uint32_t*) (gpio + GPIO_SET_DATA23);
    volatile uint32_t* gpio_clr_data_reg = (uint32_t*) (gpio + GPIO_CLR_DATA23);

    struct period_info pinfo;

    // Each sleep will be 1ms
    periodic_task_init(&pinfo, 500000000); // 0.5s

    while (1) {
        *gpio_set_data_reg |= (1 << GPIO0_42_OFFSET);
        wait_rest_of_period(&pinfo);

        *gpio_clr_data_reg |= (1 << GPIO0_42_OFFSET);
        wait_rest_of_period(&pinfo);
    }

    return NULL;
}

void* get_gpio_map() {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Could not open memory mapping");
        return NULL;
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
        return NULL;
    }

    return gpio_map;
}

int main() {
    // ================ Lock Memory =================
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("Failed to lock memory");
        exit(-1);
    }

    // =============== RT Scheduling ================

    int ret;
    pthread_attr_t attr;

    ret = pthread_attr_init(&attr);
    if (ret) {
        fprintf(stderr, "Failed to initialize pthread attributes\n");
        exit(-2);
    }

    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        fprintf(stderr, "Failed to set stack size\n");
        exit(-2);
    }

    // Set scheduling policy. This is where the task is defined as RT critical
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        fprintf(stderr, "Failed to set RT policy\n");
        exit(-2);
    }

    struct sched_param param = { 
        .sched_priority = 80 
    };
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        fprintf(stderr, "Failed to set thread priority\n");
        exit(-2);
    }

    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        fprintf(stderr, "Failed to set inheritance mode\n");
        exit(-2);
    }

    // ============= GPIO Preparation ===============

    void* gpio_map = get_gpio_map();
    if (gpio_map == NULL) {
        return EXIT_FAILURE;
    }

    volatile uint32_t* gpio_dir_reg = (uint32_t*) (gpio_map + GPIO_DIR23);

    // Configure as output
    *gpio_dir_reg &= ~(1 << GPIO0_42_OFFSET);


    // ============== Start Callback ================

    pthread_t thread;
    ret = pthread_create(&thread, &attr, toggle_gpio_task, gpio_map);
    if (ret) {
        fprintf(stderr, "Failed to create thread\n");
        exit(-2);
    }

    ret = pthread_join(thread, NULL);
    if (ret)
        fprintf(stderr, "Failed to join thread\n");

    munmap(gpio_map, BLOCK_SIZE);

    return 0;
}
