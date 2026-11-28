#pragma once
#include <stdint.h>
#include <stddef.h>
struct elf_app {const uint8_t *code; size_t size; uint64_t entry;const uint8_t *data;size_t data_files,data_mem;};
int elf_parse(const void *,size_t,struct elf_app *);
