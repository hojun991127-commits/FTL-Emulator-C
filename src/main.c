#include <stdio.h>
#include "../include/ftl.h"

int main() {
    printf("=== NAND Flash FTL Wear-Leveling Test ===\n\n");
    ftl_init();

    char temp_data[50];
    
    // 1. Cold Data 배치 (0~10번 LSN에 한 번만 쓰기)
    printf("\n[Test] 1. Cold Data 작성\n");
    for (int i = 0; i < 11; i++) {
        sprintf(temp_data, "Cold_Data_%d", i);
        ftl_write(i, temp_data);
    }

    // 2. Hot Data 집중 덮어쓰기 (12~14번 LSN에만 집중적으로 덮어쓰기)
    printf("\n[Test] 2. Hot Data 집중 덮어쓰기 (Wear-Leveling 유발)\n");
    for (int i = 0; i < 60; i++) {
        int target_lsn = 12 + (i % 3); 
        sprintf(temp_data, "Hot_Data_Version_%d", i);
        ftl_write(target_lsn, temp_data);
    }

    printf("\n================================================\n");
    return 0;
}