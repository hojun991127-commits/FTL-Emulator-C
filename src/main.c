#include <stdio.h>
#include "../include/ftl.h"

int main() {
    char buffer[512];

    printf("=== FTL L2P Mapping Test ===\n\n");
    ftl_init();

    // 1. 처음 쓰기
    printf("\n[Test 1] 논리 주소 5번에 처음 데이터 쓰기\n");
    ftl_write(5, "Hello SSD Firmware!");

    // 2. 읽기
    printf("\n[Test 2] 논리 주소 5번 데이터 읽어오기\n");
    ftl_read(5, buffer);
    printf("-> Read Data: %s\n", buffer);

    // 3. 대망의 덮어쓰기 (가장 중요!)
    printf("\n[Test 3] 논리 주소 5번에 새로운 데이터 덮어쓰기 (이사 가야 함!)\n");
    ftl_write(5, "Updated SSD Data!");

    // 4. 다시 읽기
    printf("\n[Test 4] 덮어쓴 후 논리 주소 5번 데이터 확인\n");
    ftl_read(5, buffer);
    printf("-> Read Data: %s\n", buffer);

    printf("\n============================\n");
    return 0;
}