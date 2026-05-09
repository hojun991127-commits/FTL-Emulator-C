#include <stdio.h>
#include "../include/nand_hw.h"
#include "../include/ftl.h"

// Logical to Physical Address Mapping Table
// key: LSN, value: PPN
int l2p_table[TOTAL_PAGES];

static int free_pba = 0;
static int free_page = 0;

void ftl_init(void){
    nand_init();

    // 맵핑 테이블 초기화
    for(int i = 0; i < TOTAL_PAGES; ++i){
        l2p_table[i] = -1;
    }

    free_pba = 0;
    free_page = 0;
    printf("[FTL] L2P Mapping Table Initialized.\n");
}

int ftl_read(int lsn, char* buffer){
    if(lsn < 0 || lsn >= TOTAL_PAGES)
        return -1;

    int ppn = l2p_table[lsn];
    
    // 아직 저장되지 않은 데이터였을 경우
    if(ppn == -1){
        printf("[FTL ERROR] LSN %d is Empty! (No Data Mapped)\n", lsn);
        return -1;
    }

    int pba = ppn / PAGES_PER_BLOCK;
    int page_offset = ppn % PAGES_PER_BLOCK;

    return nand_read(pba, page_offset, buffer);
}

int ftl_write(int lsn, const char* data){
    if(lsn < 0 || lsn >= TOTAL_PAGES)
        return -1;
    
    // 용량 초과 방어 로직
    if(free_pba >= NUM_BLOCKS){
        printf("[FTL ERROR] NAND is FULL! Need Garbage Collection.\n");
        return -1;
    }

    // 1. 기존 데이터가 있는지 확인
    int old_ppn = l2p_table[lsn];
    if(old_ppn != -1){
        printf("[FTL] LSN is already mapped to PPN %d. Writing to a new Physical page!\n", old_ppn);
    }

    // 2. 새 방에 데이터 쓰기
    int ret = nand_program(free_pba, free_page, data, lsn);
    if(ret != 0)
        return -1;
    
    // 3. 맵핑 테이블 업데이트
    int new_ppn = (free_pba * PAGES_PER_BLOCK) + free_page;
    l2p_table[lsn] = new_ppn;
    printf("[FTL] LSN %d mapped to PPN %d (PBA %d, Page %d)\n", lsn, new_ppn, free_pba, free_page);

    free_page++;
    if(free_page >= PAGES_PER_BLOCK){
        free_page = 0;
        free_pba++;
    }

    return 0;
}