#include <fat16/util.h>

int16_t fat16_read16(const void *p)
{
	const uint8_t *ptr = p;
	return ptr[0] | (ptr[1] << 8);
}

int32_t fat16_read32(const void *p)
{
	const uint8_t *ptr = p;
	return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}
