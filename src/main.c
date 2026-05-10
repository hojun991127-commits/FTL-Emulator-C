#include <stdio.h>
#include "../include/ftl.h"

int main() {
    printf("=== NAND Flash FTL (Garbage Collection) Test ===\n\n");
    ftl_init();

    // 시나리오: 논리 주소 0~3번에 데이터를 무한 반복해서 덮어쓰기!
    // 이렇게 하면 기존 데이터는 계속 쓰레기(Invalid)가 되고, 새 방을 계속 차지하게 됩니다.
    char temp_data[50];
    
    printf("\n[Test] 40번의 연속 덮어쓰기를 통해 디스크를 꽉 채워 GC를 유발합니다!\n");
    
    for (int i = 0; i < 40; i++) {
        int target_lsn = i % 4; // 0, 1, 2, 3번 주소만 계속 덮어씀
        sprintf(temp_data, "Data_Version_%d", i);
        ftl_write(target_lsn, temp_data);
    }

    printf("\n================================================\n");
    return 0;
}