#include <stdio.h>
#include "../include/nand_hw.h"
#include "../include/ftl.h"

// Logical to Physical Address Mapping Table
// key: LSN, value: PPN
int l2p_table[TOTAL_PAGES];

// 각 블록의 쓰레기 개수 저장
int block_invalid_count[NUM_BLOCKS];

static int free_pba = 0;
static int free_page = 0;

void ftl_init(void){
    nand_init();

    // 맵핑 테이블 초기화
    for(int i = 0; i < TOTAL_PAGES; ++i){
        l2p_table[i] = -1;
    }
    for(int i = 0; i < NUM_BLOCKS; ++i){
        block_invalid_count[i] = 0;
    }

    free_pba = 0;
    free_page = 0;
    printf("[FTL] L2P Mapping Table Initialized & Invalid Counters Initialized.\n");
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
    // 남은 블록이 하나라면 강제로 GC 실행
    if(free_pba >= NUM_BLOCKS - 1 && free_page == 0){
        ftl_garbage_collection();
    }

    // 1. 기존 데이터가 있는지 확인
    int old_ppn = l2p_table[lsn];
    if(old_ppn != -1){
        // 덮어쓰기가 발생하면 기존 page는 Garbage로 등록
        int old_pba = old_ppn / PAGES_PER_BLOCK;
        block_invalid_count[old_pba]++;
        //printf("[FTL] LSN %d Overwritten. PBA %d Invalid Count: %d", lsn, old_pba, block_invalid_count[old_pba]);
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

 void ftl_garbage_collection(void){
        printf("\n[FTL GC] Storage Limit Reached! Starting Garbage Collection...\n");

        int victim_pba = 0;
        int max_invalid = -1;

        // 1. 비울 블록 찾기
        for(int i = 0; i < NUM_BLOCKS; ++i){
            if(i == free_pba)
                continue;
            if(block_invalid_count[i] > max_invalid){
                max_invalid = block_invalid_count[i];
                victim_pba = i;
            }
        }

        printf("[FTL GC] Victim Block Selected: PBA %d (Contains %d Invalid pages)\n", victim_pba, block_invalid_count[victim_pba]);

        // 2. 블록의 유효한 페이지만 새로운 블록으로 이사
        char buffer[PAGE_SIZE];
        for(int lsn = 0; lsn < TOTAL_PAGES; ++lsn){
            int ppn = l2p_table[lsn];
            if(ppn != -1 && (ppn / PAGES_PER_BLOCK) == victim_pba){
                // ppn이 존재하고, 해당 블록이 비울 블록이라면
                nand_read(victim_pba, ppn % PAGES_PER_BLOCK, buffer);
                nand_program(free_pba, free_page, buffer, lsn);

                l2p_table[lsn] = (free_pba * PAGES_PER_BLOCK) + free_page;

                free_page++;
                if(free_page >= PAGES_PER_BLOCK){
                    free_page = 0;
                    free_pba++;
                }
            }
        }

        // 3. 해당 블록 지우기
        nand_erase(victim_pba);
        block_invalid_count[victim_pba] = 0;

        // 지워진 블록을 다음 블록으로 지정
        free_pba = victim_pba;
        free_page = 0;
    }