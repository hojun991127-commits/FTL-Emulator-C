#ifndef NAND_HW_H
#define NAND_HW_H

#include "config.h"
#include <stdint.h>

typedef enum{
    PAGE_FREE,      // 페이지 비어있음
    PAGE_VALID,     // 유효한 데이터가 들어있음
    PAGE_INVALID,   // 덮어쓰기오 Garbage가 된 상태
} PageStatus;

//1. 물리적 페이지 구조체
typedef struct{
    char data[PAGE_SIZE];   // 실제 데이터가 저장되는 공간
    PageStatus status;      // 현재 페이지 상태
    int lsn;                // Logical Sector Number (어떤 논리 주소의 데이터인지 표시하는 OOB 영역)
} NandPage;

//2. 물리적 블록 구조체
typedef struct{
    NandPage pages[PAGES_PER_BLOCK];
    int erase_count;                    // Wear-Leveling을 위해 지운 횟수 추적
} NandBlock;

// HW 제어 함수 프로토타입
void nand_init(void);                                                   // NAND 메모리 초기화
int nand_read(int pba, int page_offset, char *buffer);                  // 물리 주소 읽기
int nand_program(int pba, int page_offset, const char *data, int lsn);  // 물리 주소에 쓰기
int nand_erase(int pba);                                                // 블록 단위 지우기
int nand_get_erase_count(int pba);                                      // 블록 마모도 조회
void nand_inject_fault(int pba);                                        // 특정 블록을 고장 상태로 만듦
#endif