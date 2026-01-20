#ifndef TERMI_ARM64_MMU_H
#define TERMI_ARM64_MMU_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20

typedef struct page {
    uint64_t vaddr;
    void *data;
    uint32_t prot;
    struct page *next;
} page_t;

typedef struct {
    page_t **page_table;
    uint32_t page_table_size;
    
    uint64_t brk_start;
    uint64_t brk_current;
    
    uint64_t mmap_base;
    uint64_t mmap_current;
    
    void *tlb;
} arm64_mmu_t;

arm64_mmu_t *arm64_mmu_create(void);
void arm64_mmu_destroy(arm64_mmu_t *mmu);

int arm64_mmu_map(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t size, uint32_t prot);
int arm64_mmu_unmap(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t size);
int arm64_mmu_protect(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t size, uint32_t prot);

int arm64_mmu_read(arm64_mmu_t *mmu, uint64_t vaddr, void *buf, uint64_t size);
int arm64_mmu_write(arm64_mmu_t *mmu, uint64_t vaddr, const void *buf, uint64_t size);

int arm64_mmu_read_u8(arm64_mmu_t *mmu, uint64_t vaddr, uint8_t *val);
int arm64_mmu_read_u16(arm64_mmu_t *mmu, uint64_t vaddr, uint16_t *val);
int arm64_mmu_read_u32(arm64_mmu_t *mmu, uint64_t vaddr, uint32_t *val);
int arm64_mmu_read_u64(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t *val);

int arm64_mmu_write_u8(arm64_mmu_t *mmu, uint64_t vaddr, uint8_t val);
int arm64_mmu_write_u16(arm64_mmu_t *mmu, uint64_t vaddr, uint16_t val);
int arm64_mmu_write_u32(arm64_mmu_t *mmu, uint64_t vaddr, uint32_t val);
int arm64_mmu_write_u64(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t val);

uint64_t arm64_mmu_brk(arm64_mmu_t *mmu, uint64_t new_brk);
uint64_t arm64_mmu_mmap(arm64_mmu_t *mmu, uint64_t addr, uint64_t length, uint32_t prot, uint32_t flags);
int arm64_mmu_munmap(arm64_mmu_t *mmu, uint64_t addr, uint64_t length);

page_t *arm64_mmu_get_page(arm64_mmu_t *mmu, uint64_t vaddr);

#endif
