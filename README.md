# cnet-prac: C++ Simulation for MAC Scheduler Study

A personal repository to experiment with TTI-level downlink scheduling concepts like PF, QoS, load balancing, and trace replays.

Currently, labs `01` through `13` are fully scaffolded, with each folder containing a `실습가이드.md`, `main.cpp`, and a `build_run.sh` script. The KPI comparison tables below currently cover labs **`01` to `05`**. For labs `06` through `13`, please check the local `result.csv` or `output.txt` generated after running the simulation.

## Lab index

| Lab | Topic | Done | Date |
|-----|--------|:----:|------|
| 01 | PF, single UE per TTI, buffer depletion | [x] | 26-07-14 |
| 02 | RR / Max-CQI / PF compare (Jain, throughput) | [x] | 26-07-14 |
| 03 | PF metric + multi-UE RB split per TTI | [x] | 26-07-14 |
| 04 | QoS weight + HOL delay (M-LWDF-like), delay ON vs OFF | [x] | 26-07-14 |
| 05 | Traffic arrival, queue cap/drop; RR / Max-CQI / PF under load | [x] | 26-07-14 |
| 06 | Strict priority + GBR floor, then PF | [ ] | |
| 07 | Weight-only / delay-only / both (extends 04) | [ ] | |
| 08 | Deficit Round Robin (bytes) under load | [ ] | |
| 09 | Frequency-domain simplification | [ ] | |
| 10 | HARQ retx buffer (toy) | [ ] | |
| 11 | Fronthaul byte budget + delay | [ ] | |
| 12 | RRM cap vs MAC PF | [ ] | |
| 13 | Trace player (`trace.csv`) — reproducible KPI | [ ] | |

---
# 계획

## PF 기본
- Proportional Fair (PF)
- Max-CQI / Max Rate
- Round-Robin (RR)

## PF 변형 및 확장
- Weighted PF : metric에 QoS 가중치 (w_i) 곱함 (영상 > 백그라운드)
- GBR-aware : 최소 보장 속도(GBR) 못 채운 bearer 우선
- EPF (Enhanced PF) : 지연·버퍼·deadline 항 추가 (구현·논문마다 정의 조금씩 다름)

## QoS·지연 쪽
- M-LWDF (modified Largest Weighted Delay First) : 버퍼·지연·채널을 함께 보는 우선순위 (LTE에서 QoS 스케줄링 예로 자주 나옴)
- Priority / Strict Priority : 등급 높은 traffic 먼저 (단순하지만 낮은 등급 기아 위험)
- Token / Deficit Round Robin : 패킷 스케줄링 쪽 아이디어 (무선·유선 공통 개념)

## 자원 나누기 방식 (누구 고를지와 별개)
- Time-domain : 이번 TTI/slot에 누구에게 시간·RB 할당
- Frequency-domain : 어느 RB(주파수) 에 할당 (PF는 보통 “누구”와 함께 논의)
- MU-MIMO / multi-UE per TTI : 한 TTI에 여러 UE 동시 (당신 코드는 1 UE/TTI로 단순화)

## 실무 맥락
- RRM 정책 + 스케줄러: 상위에서 cap·QoS class 정하고, MAC 스케줄러가 그 안에서 PF 등 실행
- RIC / xApp: near-RT로 스케줄 힌트·가중치 조정 (알고리즘 이름보다 closed-loop)
- AI/ML 스케줄링: PF 대체가 아니라 채널 예측·가중치 학습 등 (JD nice-to-have)

---

# KPI 요약 (seed=42)

아래 표는 **`01`–`05`** 재실행 결과 요약. `06`–`13`은 실습 진행 후 추가될 예정.

## 01. PF 단일 스케줄 (버퍼 고갈)

| algorithm | UE0 served | UE1 served | UE2 served | total | jain | tti_until_empty |
|-----------|------------|------------|------------|-------|------|-----------------|
| PF | 5000 | 5000 | 1000 | 11000 | 0.79 | 22 |

## 02. RR / Max-CQI / PF 비교

| algorithm | UE0 served | UE1 served | UE2 served | total | jain | 비고 |
|-----------|------------|------------|------------|-------|------|------|
| RR | 14150 | 13400 | 14000 | 41550 | 1.00 | 균등 |
| Max-CQI | 22750 | 14750 | 20250 | 57750 | 0.97 | thruput↑ fairness↓ |
| PF | 18350 | 18300 | 17750 | 54400 | 1.00 | 중간~높음 |

## 03. 다중 UE RB 분할

| scenario | UE0 served | UE1 served | UE2 served | total | jain | tti_multi_ue |
|----------|------------|------------|------------|-------|------|--------------|
| PF_multi_RB | 8000 | 8000 | 8000 | 24000 | 1.00 | 5 |

## 04. QoS delay ON / OFF

| mode | UE0 picks | UE1 picks | UE2 picks | miss | served |
|------|-----------|-----------|-----------|------|--------|
| delay_ON | 12 | 15 | 12 | 0/0/0 | 6000×3 |
| delay_OFF | 11 | 16 | 12 | 0/0/0 | 6000×3 |

tip: delay OFF 시 Non-GBR(UE1) picks↑

## 05. Arrival load — RR / Max-CQI / PF

| algo | total_in | total_out | total_drop | jain_out | UE0 drop | UE1 drop | UE2 drop |
|------|----------|-----------|------------|----------|----------|----------|----------|
| RR | 198482 | 75975 | 67811 | 1.00 | 35576 | 32235 | 0 |
| Max-CQI | 198482 | 114723 | 40845 | 1.00 | 20360 | 20485 | 0 |
| PF | 198482 | 111847 | 44411 | 1.00 | 22726 | 21685 | 0 |

tip: load↑ 시 RR은 out↓·drop↑, Max-CQI는 thruput↑, PF는 중간
