#include <stdio.h>
#include <string.h>
#include "../include/nand_hw.h"

static NandBlock flash_memory[NUM_BLOCKS];
static int is_bad_block[NUM_BLOCKS];

void nand_init(void){
    for(int i = 0; i < NUM_BLOCKS; ++i){
        flash_memory[i].erase_count = 0;
        is_bad_block[i] = 0;
        for(int j = 0; j <PAGES_PER_BLOCK;++j){
            flash_memory[i].pages[j].status = PAGE_FREE;
            flash_memory[i].pages[j].lsn = -1;
            memset(flash_memory[i].pages[j].data, 0xFF, PAGE_SIZE);
        }
    }
    printf("[HW] NAND Flash %d Blocks Initialized.\n", NUM_BLOCKS);
}

void nand_inject_fault(int pba){
    if(pba >= 0 && pba < NUM_BLOCKS){
        is_bad_block[pba] = 1;
        printf("[HW ALERT] PBA %d is now physically Broken!\n", pba);
    }
}

int nand_read(int pba, int page_offset, char *buffer){
    if( pba < 0 || pba >= NUM_BLOCKS || page_offset < 0 || page_offset >= PAGES_PER_BLOCK)
        return -1;
    if(is_bad_block[pba])
        return -1;
    NandPage *target_page = &flash_memory[pba].pages[page_offset];

    memcpy(buffer, target_page->data, PAGE_SIZE);
    return target_page->lsn;
}

int nand_program(int pba, int page_offset, const char* data, int lsn){
    if( pba < 0 || pba >= NUM_BLOCKS || page_offset < 0 || page_offset >= PAGES_PER_BLOCK)
        return -1;
    if(is_bad_block[pba])
        return -1;
    NandPage *target_page = &flash_memory[pba].pages[page_offset];

    if(target_page->status != PAGE_FREE){
        printf("[HW ERROR] OVERWRITE NOT ALLOWED on PBA %d, Page %d! Must erase block first.\n", pba, page_offset);
        return -1;
    }

    memcpy(target_page->data, data, strlen(data) + 1);
    target_page->lsn = lsn;
    target_page->status = PAGE_VALID;

    printf("[HW] Programmed PBA %d, Page %d (LSN: %d)\n", pba, page_offset, lsn);
    return 0;
}

int nand_erase(int pba){
    if(pba < 0 || pba >= NUM_BLOCKS)
        return -1;
    if(is_bad_block[pba])
        return -1;
    flash_memory[pba].erase_count++;
    for(int j = 0; j < PAGES_PER_BLOCK; ++j){
        flash_memory[pba].pages[j].status = PAGE_FREE;
        flash_memory[pba].pages[j].lsn = -1;
        memset(flash_memory[pba].pages[j].data, 0xFF, PAGE_SIZE);
    }
    printf("[HW] Erased Block PBA %d (Erase Count: %d)\n", pba, flash_memory[pba].erase_count);
    return 0;
}

int nand_get_erase_count(int pba){
    if(pba < 0 || pba >= NUM_BLOCKS)
        return -1;
    return flash_memory[pba].erase_count;
}