#ifndef FTL_H
#define FTL_H

#include "config.h"

extern int l2p_table[TOTAL_PAGES];
//FTL 초기화
void ftl_init(void);

// 파일 시스템이 호출할 read/write API
int ftl_read(int lsn, char *buffer);
int ftl_write(int lsn, const char *data);

// (내부용) 가비지 컬렉션 함수
void ftl_garbage_collection(void);

// 추가: SPOR 메타데이터 스냅샷 저장
int ftl_checkpoint(void);

#endif