#ifndef TERMI_ELF_LOADER_H
#define TERMI_ELF_LOADER_H

#include <stddef.h>
#include <stdint.h>

#define ELF_MAGIC 0x464C457F

#define ET_EXEC 2
#define ET_DYN 3

#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3

#define PF_X 1
#define PF_W 2
#define PF_R 4

typedef struct {
    uint32_t e_ident_magic;
    uint8_t e_ident_class;
    uint8_t e_ident_data;
    uint8_t e_ident_version;
    uint8_t e_ident_osabi;
    uint8_t e_ident_padding[8];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

typedef struct {
    uint64_t entry_point;
    uint64_t load_base;
    uint64_t stack_top;
    int has_interpreter;
    char interpreter_path[256];
    uint64_t interp_base;
    uint64_t interp_entry;
} elf_load_info_t;

int elf_load(const char *path, void *mmu, elf_load_info_t *info);
int elf_load_memory(const void *elf_data, size_t size, void *mmu, elf_load_info_t *info);
int elf_load_memory_with_fs(const void *elf_data, size_t size, void *mmu, elf_load_info_t *info, void *fakefs);

#endif
