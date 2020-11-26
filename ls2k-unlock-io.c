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
#define GMAC1_SEL BIT(3)
#define PWM0_SEL BIT(12)
#define PWM1_SEL BIT(13)
#define PWM2_SEL BIT(14)
#define PWM3_SEL BIT(15)


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

static int write_sel_register(void) {
	int ret = 0;
	int fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (fd < 0) {
		perror("/dev/mem");
		ret = 1;
		return ret;
	}

	void *addr = (void *)0x1fe10000;
	off_t offset = 0x420;

	void *mem = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)addr);

	if (mem == MAP_FAILED) {
		perror("mmap");
		ret = 1;
		goto fail;
	}

	uint64_t *reg = mem + offset;

#ifdef DEBUG
	printf("%p -> %p\n", addr + offset, *reg);
#endif

	// Set GMAC1_SEL to 0 (GPIO)
	*reg &= ~GMAC1_SEL;

	// Set PWM{0..3}_SEL to 1 (PWM)
	*reg |= PWM0_SEL;
	*reg |= PWM1_SEL;
	*reg |= PWM2_SEL;
	*reg |= PWM3_SEL;

fail:
	close(fd);
	return ret;
}

int main(int argc, char **argv) {
	if (check_platform()) {
		fprintf(stderr, "Platform mismatch. This program only works on Loongson Pi EDU platforms.");
		return EXIT_FAILURE;
	}
	if (write_sel_register()) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
