#include <stdio.h>
#include <string.h>
#include "../include/ftl.h"
#include "../include/nand_hw.h"

int main() {
    printf("=== NAND Flash FTL Sudden Power Off Recovery (SPOR) Test ===\n\n");

    // 1. 첫 번째 부팅 및 데이터 쓰기
    printf("--- [Phase 1] First Boot & Data Write ---\n");
    ftl_init();

    ftl_write(0, "User_Data_LSN_0");
    ftl_write(1, "User_Data_LSN_1");

    // 💾 전원이 꺼지기 전 매핑 테이블 스냅샷 저장
    ftl_checkpoint();
    
    printf("Simulating Sudden Power Off... (Data in RAM is completely lost)\n");
    printf("================================================================\n\n");


    // 2. 두 번째 부팅 (재부팅 시뮬레이션)
    printf("--- [Phase 2] Reboot & SPOR Trigger ---\n");
    ftl_init(); 

    // 3. 복구 검증 (지도가 잘 살아났는지 Read 테스트)
    printf("\n--- [Phase 3] Verification ---\n");
    char read_buffer[4096];
    
    printf("[Test] Reading LSN 0 after recovery...\n");
    int ppn_0 = l2p_table[0];
    if (ppn_0 != -1) {
        nand_read(ppn_0 / PAGES_PER_BLOCK, ppn_0 % PAGES_PER_BLOCK, read_buffer);
        printf("[Result] LSN 0 Data: %s\n", read_buffer);
    } else {
        printf("[Result] Fail : LSN 0 Mapping lost.\n");
    }

    printf("[Test] Reading LSN 1 after recovery...\n");
    int ppn_1 = l2p_table[1];
    if (ppn_1 != -1) {
        nand_read(ppn_1 / PAGES_PER_BLOCK, ppn_1 % PAGES_PER_BLOCK, read_buffer);
        printf("[Result] LSN 1 Data: %s\n", read_buffer);
    } else {
        printf("[Result] Fail : LSN 1 Mapping lost.\n");
    }

    printf("\n================================================================\n");
    return 0;
}