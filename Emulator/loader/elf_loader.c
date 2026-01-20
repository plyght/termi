#include "elf_loader.h"
#include "../mmu/mmu.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define STACK_SIZE (8 * 1024 * 1024)
#define STACK_TOP 0x7FFFFFFFF000ULL

static uint32_t pf_to_prot(uint32_t flags)
{
    uint32_t prot = 0;
    if (flags & PF_R)
        prot |= PROT_READ;
    if (flags & PF_W)
        prot |= PROT_WRITE;
    if (flags & PF_X)
        prot |= PROT_EXEC;
    return prot;
}

int elf_load_memory(const void *elf_data, size_t size, void *mmu_ptr, elf_load_info_t *info)
{
    arm64_mmu_t *mmu = (arm64_mmu_t *)mmu_ptr;
    const uint8_t *data = (const uint8_t *)elf_data;

    printf("[ELF Loader] Loading ELF from memory, size=%zu\n", size);

    if (size < sizeof(elf64_ehdr_t)) {
        printf("[ELF Loader] ERROR: Size too small (%zu < %zu)\n", size, sizeof(elf64_ehdr_t));
        return -1;
    }

    elf64_ehdr_t *ehdr = (elf64_ehdr_t *)data;

    printf("[ELF Loader] Magic: 0x%08x (expected 0x%08x)\n", ehdr->e_ident_magic, ELF_MAGIC);
    if (ehdr->e_ident_magic != ELF_MAGIC) {
        printf("[ELF Loader] ERROR: Invalid ELF magic\n");
        return -1;
    }

    printf("[ELF Loader] Class: %d (expected 2 for 64-bit)\n", ehdr->e_ident_class);
    if (ehdr->e_ident_class != 2) {
        printf("[ELF Loader] ERROR: Not 64-bit ELF\n");
        return -1;
    }

    printf("[ELF Loader] Machine: %d (expected 183 for ARM64)\n", ehdr->e_machine);
    if (ehdr->e_machine != 183) {
        printf("[ELF Loader] ERROR: Not ARM64\n");
        return -1;
    }

    info->entry_point = ehdr->e_entry;
    info->load_base = 0;
    info->has_interpreter = 0;

    printf("[ELF Loader] Entry point: 0x%llx, Program headers: %d\n", 
           (unsigned long long)ehdr->e_entry, ehdr->e_phnum);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(data + ehdr->e_phoff + i * ehdr->e_phentsize);

        printf("[ELF Loader] Program header %d: type=%d\n", i, phdr->p_type);

        if (phdr->p_type == PT_INTERP) {
            info->has_interpreter = 1;
            size_t interp_len = phdr->p_filesz;
            if (interp_len > sizeof(info->interpreter_path) - 1) {
                interp_len = sizeof(info->interpreter_path) - 1;
            }
            memcpy(info->interpreter_path, data + phdr->p_offset, interp_len);
            info->interpreter_path[interp_len] = '\0';
            printf("[ELF Loader] Found interpreter: %s\n", info->interpreter_path);
        }

        if (phdr->p_type == PT_LOAD) {
            uint64_t vaddr = phdr->p_vaddr;
            uint64_t memsz = phdr->p_memsz;
            uint64_t filesz = phdr->p_filesz;
            uint32_t prot = pf_to_prot(phdr->p_flags);

            printf("[ELF Loader] LOAD segment: vaddr=0x%llx, memsz=%llu, filesz=%llu, prot=0x%x\n",
                   (unsigned long long)vaddr, (unsigned long long)memsz, 
                   (unsigned long long)filesz, prot);

            // Map with write permission during loading, we'll fix protection later if needed
            uint32_t load_prot = prot | PROT_WRITE;
            if (arm64_mmu_map(mmu, vaddr, memsz, load_prot) < 0) {
                printf("[ELF Loader] ERROR: MMU map failed for vaddr=0x%llx\n", (unsigned long long)vaddr);
                return -1;
            }

            if (filesz > 0) {
                if (arm64_mmu_write(mmu, vaddr, data + phdr->p_offset, filesz) < 0) {
                    printf("[ELF Loader] ERROR: MMU write failed for vaddr=0x%llx\n", (unsigned long long)vaddr);
                    return -1;
                }
                printf("[ELF Loader] Wrote %llu bytes to vaddr=0x%llx\n", 
                       (unsigned long long)filesz, (unsigned long long)vaddr);
            }

            if (memsz > filesz) {
                uint8_t zero = 0;
                for (uint64_t j = filesz; j < memsz; j++) {
                    arm64_mmu_write(mmu, vaddr + j, &zero, 1);
                }
                printf("[ELF Loader] Zeroed %llu bytes at vaddr=0x%llx\n", 
                       (unsigned long long)(memsz - filesz), (unsigned long long)(vaddr + filesz));
            }
        }
    }

    printf("[ELF Loader] Mapping stack: top=0x%llx, size=%d\n", 
           (unsigned long long)STACK_TOP, STACK_SIZE);

    info->stack_top = STACK_TOP;
    if (arm64_mmu_map(mmu, STACK_TOP - STACK_SIZE, STACK_SIZE, PROT_READ | PROT_WRITE) < 0) {
        printf("[ELF Loader] ERROR: Stack mapping failed\n");
        return -1;
    }

    printf("[ELF Loader] ELF loaded successfully\n");
    return 0;
}

int elf_load(const char *path, void *mmu, elf_load_info_t *info)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return -1;
    }

    void *data = malloc(st.st_size);
    if (!data) {
        close(fd);
        return -1;
    }

    ssize_t n = read(fd, data, st.st_size);
    close(fd);

    if (n != st.st_size) {
        free(data);
        return -1;
    }

    int ret = elf_load_memory(data, st.st_size, mmu, info);
    free(data);

    return ret;
}
