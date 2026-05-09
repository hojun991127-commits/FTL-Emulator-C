#ifndef FTL_H
#define FTL_H

#include "config.h"

//FTL 초기화
void ftl_init(void);

// 파일 시스템이 호출할 read/write API
int ftl_read(int lsn, char *buffer);
int ftl_write(int lsn, const char *data);

// (내부용) 가비지 컬렉션 함수
void ftl_garbage_collection(void);

#endif