#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#define MODEL "Loongson-2K-SOC-1w-V1.1-EDU_UDB"

#define BIT(n) (UINT64_C(1) << (n))

#define GS264E_GENERAL_CONF_REG_0 (void*)0x1fe10420
#define GS264E_GENERAL_CONF_REG_1 (void*)0x1fe10428
#define GS264E_GENERAL_CONF_REG_2 (void*)0x1fe10430
#define GS264E_GMAC1_SEL BIT(3)
#define GS264E_PWM0_SEL BIT(12)
#define GS264E_PWM1_SEL BIT(13)
#define GS264E_PWM2_SEL BIT(14)
#define GS264E_PWM3_SEL BIT(15)
#define GS264E_UART0_ENABLE_2WIRE 0xf

#define PGBITS 12
#define PGSIZE (1UL << PGBITS)
#define PGMASK (PGSIZE - 1)

#define register_write(fd, addr, size, data) register_rw(fd, addr, size, data, DIR_WRITE)
#define register_read(fd, addr, size, data) register_rw(fd, addr, size, data, DIR_READ)

enum direction_t { DIR_READ, DIR_WRITE };

static int check_platform(void) {
	int ret = 0;
	FILE *fp = fopen("/sys/devices/virtual/dmi/id/board_name", "r");
	char model[sizeof(MODEL)];

	if (fp == NULL) {
		return 1;
	}

	if (fgets(model, sizeof(model), fp)) {
#ifdef DEBUG
		printf("Model = %s\n", model);
#endif
		ret = (strncmp(MODEL, model, sizeof(model)) != 0);
	}

	fclose(fp);
	return ret;
}


static int register_rw(int fd, void* addr, size_t size, void* data, enum direction_t direction) {
	off_t addr_to_map = (off_t)addr & ~PGMASK;
	void *mem = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr_to_map);
	if (mem == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	void *reg = mem + ((off_t)addr & PGMASK);

	if (direction == DIR_READ)
		memcpy(data, reg, size);
	else
		memcpy(reg, data, size);

	munmap(mem, size);
	return 0;
}

int main(int argc, char **argv) {
	if (check_platform()) {
		fprintf(stderr, "Platform mismatch. This program only works on Loongson Pi EDU platforms.");
		return EXIT_FAILURE;
	}

	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("/dev/mem");
		exit(EXIT_FAILURE);
	}
	uint64_t gconf_reg_0, gconf_reg_1;

	if (register_read(fd, GS264E_GENERAL_CONF_REG_0, sizeof(uint64_t), &gconf_reg_0) > 0) goto fail;
	if (register_read(fd, GS264E_GENERAL_CONF_REG_1, sizeof(uint64_t), &gconf_reg_1) > 0) goto fail;

#ifdef DEBUG
	printf("gconf_reg_0 = 0x%lx\n", gconf_reg_0);
	printf("gconf_reg_1 = 0x%lx\n", gconf_reg_1);
#endif

	// Set GMAC1_SEL to 0
	gconf_reg_0 &= ~GS264E_GMAC1_SEL;

	// Set PWM{0..3}_SEL to 1
	gconf_reg_0 |= GS264E_PWM0_SEL | GS264E_PWM1_SEL | GS264E_PWM2_SEL | GS264E_PWM3_SEL;

	// Set UART0_ENABLE to 4'b1111
	gconf_reg_1 |= GS264E_UART0_ENABLE_2WIRE;

	if (register_write(fd, GS264E_GENERAL_CONF_REG_0, sizeof(uint64_t), &gconf_reg_0) > 0) goto fail;
	if (register_write(fd, GS264E_GENERAL_CONF_REG_1, sizeof(uint64_t), &gconf_reg_1) > 0) goto fail;

#ifdef DEBUG
	printf("register values updated\n");
	printf("gconf_reg_0 = 0x%lx\n", gconf_reg_0);
	printf("gconf_reg_1 = 0x%lx\n", gconf_reg_1);
#endif

fail:
	close(fd);
	return EXIT_SUCCESS;
}
