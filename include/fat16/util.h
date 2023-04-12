#include <linux/fs.h>

// Read a little-endian, 16-bit value from p
int16_t fat16_read16(const void *p);

// Read a little-endian, 32-bit value from p
int32_t fat16_read32(const void *p);
