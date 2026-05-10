# 💾 NAND-FTL-Simulator

> NAND Flash Memory Translation Layer (FTL) Simulation and Optimization in C/C++

## 📌 Project Overview
본 프로젝트는 물리적인 NAND 플래시 메모리의 하드웨어적 제약(Erase-before-write, 덮어쓰기 불가)을 소프트웨어적으로 해결하기 위한 **FTL(Flash Translation Layer)** 에뮬레이터이다. 
단순한 데이터 읽기/쓰기를 넘어, 실제 SSD 펌웨어에서 발생하는 **Out-of-Place Update** 매커니즘과 **Garbage Collection** (GC) 알고리즘을 C언어로 직접 구현하여 시스템의 안정성과 스토리지 수명을 관리하는 과정을 검증한다.

## 🛠️ Tech Stack
- **Language:** C / C++
- **Build Tool:** Make (GCC)
- **Environment:** Linux (WSL)

## 🏗️ Core Architecture (3-Layer)
본 프로젝트는 실무 펌웨어 구조를 반영하여 3개의 계층으로 추상화하여 설계되었다.
1. **Top Layer (`main.c`):** 파일 시스템의 논리적 읽기/쓰기 명령(LSN 기반) 생성 및 시나리오 테스트
2. **Middle Layer (`ftl.c`):** L2P(Logical-to-Physical) 맵핑 테이블 관리 및 가비지 컬렉션(Greedy 알고리즘) 수행
3. **Bottom Layer (`nand_hw.c`):** 물리적 NAND 칩(Block/Page 단위)의 동작 모사 및 덮어쓰기 에러(Overwrite 방어선) 제어

## 💡 Key Features & Troubleshooting

### 1. Out-of-Place Update 및 덮어쓰기 방어
* **문제:** NAND 플래시는 한 번 데이터가 쓰인 Page에 덮어쓰기가 불가능함.
* **해결:** 하드웨어 단에서 덮어쓰기 시도를 에러(`OVERWRITE NOT ALLOWED`)로 방어. FTL 계층에서 기존 데이터는 쓰레기(Invalid) 처리하고, 새로운 Free Page에 데이터를 쓴 뒤 L2P 맵핑 테이블을 갱신하는 Out-of-Place Update 로직 구현.

### 2. 가비지 컬렉션 (Garbage Collection) 및 성능 한계 극복
* **문제:** 지속적인 덮어쓰기 발생 시 Invalid Page가 누적되어 스토리지 용량이 고갈(`Storage Limit Reached`)되고 시스템이 다운되는 현상 발생.
* **해결:** 여유 블록이 1개 이하로 떨어질 시 **Greedy 알고리즘 기반의 GC** 강제 실행.
    1. 각 블록의 Invalid 카운트를 추적하여 가장 쓰레기가 많은 **Victim Block(희생 블록)** 선정.
    2. Victim Block 내의 유효한(Valid) 데이터만 새로운 공간으로 복사(Copy).
    3. Victim Block을 통째로 지워(Erase) 다시 Free Block으로 확보하여 시스템 연속성 보장.

### 3. Wear-Leveling (마모도 평준화) 알고리즘 도입
* **문제:** 특정 LSN에만 쓰기가 집중되는 Hot Data 패턴 발생 시, 매핑된 특정 물리 블록(PBA)만 수명이 급격히 깎이는 편중 현상 발생.
* **해결:** NAND의 수명을 전체적으로 상향 평준화하기 위해 두 가지 전략을 적용함.
    1. **Dynamic Wear-Leveling:** Free Block 할당 시 단순히 순차 할당하지 않고, 가용한 블록 중 **마모도(Erase Count)가 가장 낮은 블록**을 우선 탐색하여 할당 (`allocate_free_block`).
    2. **Static Wear-Leveling:** 블록 간 마모도 격차가 임계값(`WL_THRESHOLD`)을 초과할 경우, 업데이트가 없는 Cold Data를 강제로 새로운 블록으로 이주시킴. 비워진 건강한 블록은 다시 Hot Data를 받을 수 있도록 상태를 순환시킴.

### 4. 시나리오 기반 GC 성능 최적화 검증 (Zero-Copy GC)
* **현상:** 특정 LSN 영역만 지속적으로 덮어쓰는 극단적인 Hot Data 테스트 시나리오(`main.c`) 수행.
* **결과 검증:** 타겟이 된 물리 블록 내부의 모든 페이지가 무효화(Invalid) 처리됨에 따라, GC 실행 시 유효 데이터 대피를 위한 추가적인 읽기/쓰기(nand_read, nand_program) 오버헤드가 **0번** 발생함을 확인.
* **의의:** 정석적인 Greedy 기반 GC 알고리즘이 특정 워크로드(Workload)와 맞물렸을 때, 불필요한 데이터 마이그레이션이 생략되며 **쓰기 증폭(WAF)이 최소화되는 Zero-Copy 소거**가 자연스럽게 동작함을 시뮬레이션으로 입증함.

## 🚀 Execution & Log (가비지 컬렉션 동작 확인)
무한 덮어쓰기를 통해 디스크를 꽉 채워 강제로 GC를 유발하는 테스트 결과

```bash
$ make
$ ./ftl_sim

=== NAND Flash FTL (Garbage Collection) Test ===

[FTL] L2P Mapping Table & Invalid Counters Initialized.

[Test] 40번의 연속 덮어쓰기를 통해 디스크를 꽉 채워 GC를 유발합니다!

[FTL GC] Storage Limit Reached! Starting Garbage Collection...
[FTL GC] Victim Block Selected: PBA 0 (Contains 4 Invalid pages)
[HW] Erased Block PBA 0 (Erase Count: 1)
[FTL GC] Garbage Collection Completed! Space Recovered.