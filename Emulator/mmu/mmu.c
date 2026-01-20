#include "mmu.h"
#include <stdlib.h>
#include <string.h>

#define PAGE_TABLE_SIZE 65536

arm64_mmu_t *arm64_mmu_create(void)
{
    arm64_mmu_t *mmu = calloc(1, sizeof(arm64_mmu_t));
    if (!mmu)
        return NULL;

    mmu->page_table = calloc(PAGE_TABLE_SIZE, sizeof(page_t *));
    if (!mmu->page_table) {
        free(mmu);
        return NULL;
    }
    mmu->page_table_size = PAGE_TABLE_SIZE;

    mmu->brk_start = 0x40000000;
    mmu->brk_current = mmu->brk_start;
    mmu->mmap_base = 0x50000000;
    mmu->mmap_current = mmu->mmap_base;

    return mmu;
}

void arm64_mmu_destroy(arm64_mmu_t *mmu)
{
    if (!mmu)
        return;

    for (uint32_t i = 0; i < mmu->page_table_size; i++) {
        page_t *page = mmu->page_table[i];
        while (page) {
            page_t *next = page->next;
            free(page->data);
            free(page);
            page = next;
        }
    }

    free(mmu->page_table);
    free(mmu);
}

static uint32_t page_hash(uint64_t vaddr)
{
    return (vaddr >> PAGE_SHIFT) % PAGE_TABLE_SIZE;
}

page_t *arm64_mmu_get_page(arm64_mmu_t *mmu, uint64_t vaddr)
{
    uint64_t page_addr = vaddr & PAGE_MASK;
    uint32_t hash = page_hash(page_addr);

    page_t *page = mmu->page_table[hash];
    while (page) {
        if (page->vaddr == page_addr) {
            return page;
        }
        page = page->next;
    }

    return NULL;
}

int arm64_mmu_map(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t size, uint32_t prot)
{
    uint64_t start = vaddr & PAGE_MASK;
    uint64_t end = (vaddr + size + PAGE_SIZE - 1) & PAGE_MASK;

    for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
        if (arm64_mmu_get_page(mmu, addr)) {
            continue;
        }

        page_t *page = calloc(1, sizeof(page_t));
        if (!page)
            return -1;

        page->data = calloc(1, PAGE_SIZE);
        if (!page->data) {
            free(page);
            return -1;
        }

        page->vaddr = addr;
        page->prot = prot;

        uint32_t hash = page_hash(addr);
        page->next = mmu->page_table[hash];
        mmu->page_table[hash] = page;
    }

    return 0;
}

int arm64_mmu_read(arm64_mmu_t *mmu, uint64_t vaddr, void *buf, uint64_t size)
{
    for (uint64_t i = 0; i < size; i++) {
        uint64_t addr = vaddr + i;
        page_t *page = arm64_mmu_get_page(mmu, addr);

        if (!page || !(page->prot & PROT_READ)) {
            return -1;
        }

        uint64_t offset = addr & ~PAGE_MASK;
        ((uint8_t *)buf)[i] = ((uint8_t *)page->data)[offset];
    }

    return 0;
}

int arm64_mmu_write(arm64_mmu_t *mmu, uint64_t vaddr, const void *buf, uint64_t size)
{
    for (uint64_t i = 0; i < size; i++) {
        uint64_t addr = vaddr + i;
        page_t *page = arm64_mmu_get_page(mmu, addr);

        if (!page || !(page->prot & PROT_WRITE)) {
            return -1;
        }

        uint64_t offset = addr & ~PAGE_MASK;
        ((uint8_t *)page->data)[offset] = ((const uint8_t *)buf)[i];
    }

    return 0;
}

int arm64_mmu_read_u8(arm64_mmu_t *mmu, uint64_t vaddr, uint8_t *val)
{
    return arm64_mmu_read(mmu, vaddr, val, 1);
}

int arm64_mmu_read_u16(arm64_mmu_t *mmu, uint64_t vaddr, uint16_t *val)
{
    return arm64_mmu_read(mmu, vaddr, val, 2);
}

int arm64_mmu_read_u32(arm64_mmu_t *mmu, uint64_t vaddr, uint32_t *val)
{
    return arm64_mmu_read(mmu, vaddr, val, 4);
}

int arm64_mmu_read_u64(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t *val)
{
    return arm64_mmu_read(mmu, vaddr, val, 8);
}

int arm64_mmu_write_u8(arm64_mmu_t *mmu, uint64_t vaddr, uint8_t val)
{
    return arm64_mmu_write(mmu, vaddr, &val, 1);
}

int arm64_mmu_write_u16(arm64_mmu_t *mmu, uint64_t vaddr, uint16_t val)
{
    return arm64_mmu_write(mmu, vaddr, &val, 2);
}

int arm64_mmu_write_u32(arm64_mmu_t *mmu, uint64_t vaddr, uint32_t val)
{
    return arm64_mmu_write(mmu, vaddr, &val, 4);
}

int arm64_mmu_write_u64(arm64_mmu_t *mmu, uint64_t vaddr, uint64_t val)
{
    return arm64_mmu_write(mmu, vaddr, &val, 8);
}

uint64_t arm64_mmu_brk(arm64_mmu_t *mmu, uint64_t new_brk)
{
    if (new_brk == 0) {
        return mmu->brk_current;
    }

    if (new_brk < mmu->brk_start) {
        return mmu->brk_current;
    }

    if (new_brk > mmu->brk_current) {
        uint64_t size = new_brk - mmu->brk_current;
        if (arm64_mmu_map(mmu, mmu->brk_current, size, PROT_READ | PROT_WRITE) < 0) {
            return mmu->brk_current;
        }
    }

    mmu->brk_current = new_brk;
    return mmu->brk_current;
}

uint64_t arm64_mmu_mmap(arm64_mmu_t *mmu, uint64_t addr, uint64_t length, uint32_t prot,
                        uint32_t flags)
{
    if (addr == 0) {
        addr = mmu->mmap_current;
        mmu->mmap_current += (length + PAGE_SIZE - 1) & PAGE_MASK;
    }

    if (arm64_mmu_map(mmu, addr, length, prot) < 0) {
        return (uint64_t)-1;
    }

    if (flags & MAP_ANONYMOUS) {
        for (uint64_t i = 0; i < length; i += PAGE_SIZE) {
            page_t *page = arm64_mmu_get_page(mmu, addr + i);
            if (page && page->data) {
                memset(page->data, 0, PAGE_SIZE);
            }
        }
    }

    return addr;
}

int arm64_mmu_munmap(arm64_mmu_t *mmu, uint64_t addr, uint64_t length)
{
    uint64_t start = addr & PAGE_MASK;
    uint64_t end = (addr + length + PAGE_SIZE - 1) & PAGE_MASK;

    for (uint64_t page_addr = start; page_addr < end; page_addr += PAGE_SIZE) {
        uint32_t hash = page_hash(page_addr);
        page_t **prev = &mmu->page_table[hash];
        page_t *page = *prev;

        while (page) {
            if (page->vaddr == page_addr) {
                *prev = page->next;
                free(page->data);
                free(page);
                break;
            }
            prev = &page->next;
            page = page->next;
        }
    }

    return 0;
}
