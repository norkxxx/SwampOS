#pragma once
#include <stdint.h>

enum { FIXTURE_SECTORS = 8192, FIXTURE_BLOCK = 512 };
enum fixture_op { FIXTURE_READ, FIXTURE_WRITE, FIXTURE_FLUSH };
/* One explicitly attached Q35 disk, sector 0 label and two record slots. */
const char *fixture_open(void);
int fixture_io(enum fixture_op op, unsigned lba, void *sector);
void fixture_close(void);
