#include <stdio.h>
#include "../include/nand_hw.h"
#include "../include/ftl.h"

#define WL_THRESHOLD 3  // 정적 웨어 레벨링 발동 임계값
#define INF 999999;

// Logical to Physical Address Mapping Table
// key: LSN, value: PPN
int l2p_table[TOTAL_PAGES];

// 각 블록의 쓰레기 개수 저장
int block_invalid_count[NUM_BLOCKS];

// 추가: 각 블록의 valid 페이지 저장
int block_valid_count[NUM_BLOCKS];

// 추가: 배드 블록 테이블
int block_bad_flag[NUM_BLOCKS];

static int free_pba = 0;
static int free_page = 0;

// [Dynamic WL] 빈 블록 중 가장 P/E 사이클이 낮은 블록을 찾는 함수
static int allocate_free_block(){
    int target_pba = -1;
    int min_erase = INF;

    for(int i = 0; i < NUM_BLOCKS; ++i){
        // valid / invalid 모두 0이면 완전히 비어있는 free 블록
        if(block_bad_flag[i] == 0 &&  block_invalid_count[i] == 0 && block_valid_count[i] == 0 && i != free_pba){
            int erase_cnt = nand_get_erase_count(i);
            if(erase_cnt < min_erase){
                min_erase = erase_cnt;
                target_pba = i;
            }
        }
    }
    return target_pba;
}


void ftl_init(void){
    nand_init();

    // 맵핑 테이블 초기화
    for(int i = 0; i < TOTAL_PAGES; ++i){
        l2p_table[i] = -1;
    }
    for(int i = 0; i < NUM_BLOCKS; ++i){
        block_invalid_count[i] = 0;
        block_valid_count[i] = 0;
        block_bad_flag[i] = 0;
    }

    free_pba = 0;
    free_page = 0;
    printf("[FTL] L2P Mapping Table & Counters & BBT Initialized.\n");
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

// [Static WL] 마모도 불균형 검사 및 Cold Data 강제 이주
static void check_static_wear_leveling(){
    int max_erase = -1;
    int min_erase = INF;
    int max_pba = -1;
    int min_pba = -1;

    for(int i = 0; i < NUM_BLOCKS; ++i){
        int erase_cnt = nand_get_erase_count(i);
        if(erase_cnt > max_erase){
            max_erase = erase_cnt;
            max_pba = i;
        }
        if(erase_cnt < min_erase){
            min_erase = erase_cnt;
            min_pba = i;
        }
    }

    if((max_erase - min_erase) >= WL_THRESHOLD && block_valid_count[min_pba] > 0){
        printf("\n[FTL WL] Static Wear-Leveling Triggered! (Max PBA %d: %d, Min PBA %d: %d)\n", max_pba, max_erase, min_pba, min_erase);
        
        // Cold Data (min_ppa)를 강제로 free_pba로 이주
        char buffer[PAGE_SIZE];
        for(int lsn = 0; lsn < TOTAL_PAGES; ++lsn){
            int ppn = l2p_table[lsn];
            if(ppn != -1 && (ppn / PAGES_PER_BLOCK) == min_pba){
                nand_read(min_pba, (ppn % PAGES_PER_BLOCK), buffer);
                nand_program(free_pba, free_page, buffer, lsn);
                l2p_table[lsn] = (free_pba * PAGES_PER_BLOCK) + free_page;

                block_valid_count[free_pba]++;
                free_page++;
                if(free_page >= PAGES_PER_BLOCK){
                    free_page = 0;
                    free_pba = allocate_free_block();
                }
            }
        }
        nand_erase(min_pba);
        block_valid_count[min_pba] = 0;
        block_invalid_count[min_pba] = 0;
    }
}


int ftl_write(int lsn, const char* data){
    if(lsn < 0 || lsn >= TOTAL_PAGES)
        return -1;
    
    // 용량 초과 방어 로직
    // 블록 고갈 시 GC 실행
    if(allocate_free_block() == -1 && free_page == 0){
        ftl_garbage_collection();
    }

    // 1. 기존 데이터가 있는지 확인
    int old_ppn = l2p_table[lsn];
    if(old_ppn != -1){
        // 덮어쓰기가 발생하면 기존 page는 Garbage로 등록
        int old_pba = old_ppn / PAGES_PER_BLOCK;
        block_invalid_count[old_pba]++;
        block_valid_count[old_pba]--;
        //printf("[FTL] LSN %d Overwritten. PBA %d Invalid Count: %d", lsn, old_pba, block_invalid_count[old_pba]);
    }

    // 2. 새 방에 데이터 쓰기
    int write_status = nand_program(free_pba, free_page, data, lsn);
    // 쓰기 시도 및 실패 감지
    if(write_status == -1){
        // BBM 로직 수행
        printf("\n[FTL BBM ERROR] Runtime Bad Block detected at PBA %d!\n", free_pba);
        block_bad_flag[free_pba] = 1;   // BBT에 불량 블록으로 영구 등록

        // 새 예비 블록(OP) 긴급 할당
        free_pba = allocate_free_block();
        free_page = 0;

        printf("[FTL BBM Rescue] Remapping LSN %d to new PBA %d...\n\n", lsn, free_pba);
        // 새 블록에 데이터 다시 쓰기
        nand_program(free_pba, free_page, data, lsn);
    }
        
    
    // 3. 맵핑 테이블 업데이트
    int new_ppn = (free_pba * PAGES_PER_BLOCK) + free_page;
    l2p_table[lsn] = new_ppn;
    block_valid_count[free_pba]++;
    printf("[FTL] LSN %d mapped to PPN %d (PBA %d, Page %d)\n", lsn, new_ppn, free_pba, free_page);

    free_page++;
    if(free_page >= PAGES_PER_BLOCK){
        free_page = 0;
        free_pba = allocate_free_block();   // 단순 증가가 아닌 최적의 블록 탐색 (Dynamic Wear-Leveling))
        check_static_wear_leveling();       // 블록이 바뀔 때마다 불균형 검사
    }

    return 0;
}

 void ftl_garbage_collection(void){
        printf("\n[FTL GC] Storage Limit Reached! Starting Garbage Collection...\n");

        int victim_pba = -1;
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
                block_valid_count[free_pba]++;

                free_page++;
                if(free_page >= PAGES_PER_BLOCK){
                    free_page = 0;
                    free_pba = allocate_free_block();
                }
            }
        }

        // 3. 해당 블록 지우기
        nand_erase(victim_pba);
        block_invalid_count[victim_pba] = 0;
        block_valid_count[victim_pba] = 0;

        // GC 이후 남은 빈 블록 중 최적의 블록으로 다시 세팅
        if(free_page == 0)
            free_pba = allocate_free_block();
    }