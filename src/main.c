#include <stdio.h>
#include "../include/ftl.h"
#include "../include/nand_hw.h" // 결함 주입 함수를 쓰기 위해 추가

int main() {
    printf("=== NAND Flash FTL Wear-Leveling Test ===\n\n");
    ftl_init();

    // 의도적인 하드웨어 결함 주입
    printf("\n[Test] Injecting physical fault to PBA 2...\n");
    nand_inject_fault(2);
    printf("================================================\n\n");

    char temp_data[50];

    //데이터를 연속으로 쓰면서 PBA 0 -> PBA 1 -> PBA 2(고장!) 차례가 올 때를 관찰
    printf("[Test] Writing sequential data to trigger Bad Block...\n");

    for(int i = 0; i < 12; ++i){
        sprintf(temp_data, "Data_Chunk_%d", i);
        ftl_write(i, temp_data);
    }
    
    printf("\n================================================\n");
    return 0;
}