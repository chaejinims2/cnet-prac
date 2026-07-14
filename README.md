# 실습 후보

## 기본
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

각 폴더 `result.csv` / `output.txt` 기준. 재실행: `cd NN && ./build_run.sh`

## 01. PF 단일 스케줄 (버퍼 고갈)

| algorithm | UE0 served | UE1 served | UE2 served | total | jain | tti_until_empty |
|-----------|------------|------------|------------|-------|------|-----------------|
| PF | 5000 | 5000 | 1000 | 11000 | 0.79 | 23 |

## 02. RR / Max-CQI / PF 비교

| algorithm | UE0 served | UE1 served | UE2 served | total | jain | 비고 |
|-----------|------------|------------|------------|-------|------|------|
| RR | 11600 | 15100 | 13550 | 40250 | 0.99 | 균등 |
| Max-CQI | 14850 | 25600 | 20400 | 60850 | 0.96 | thruput↑ fairness↓ |
| PF | 16400 | 22050 | 19000 | 57450 | 0.99 | 중간~높음 |

## 03. 다중 UE RB 분할

| scenario | UE0 served | UE1 served | UE2 served | total | jain | tti |
|----------|------------|------------|------------|-------|------|-----|
| PF_multi_RB | 8000 | 8000 | 8000 | 24000 | 1.00 | 50 |

## 04. QoS delay ON / OFF

| mode | UE0 picks | UE1 picks | UE2 picks | miss | served |
|------|-----------|-----------|-----------|------|--------|
| delay_ON | 12 | 13 | 13 | 0/0/0 | 6000×3 |
| delay_OFF | 11 | 15 | 12 | 0/0/0 | 6000×3 |

tip: delay OFF 시 Non-GBR(UE1) picks↑

## 05. Arrival load — RR / Max-CQI / PF

| algo | total_in | total_out | total_drop | jain_out | UE0 drop | UE1 drop | UE2 drop |
|------|----------|-----------|------------|----------|----------|----------|----------|
| RR | 200260 | 78450 | 69356 | 1.00 | 35379 | 33977 | 0 |
| Max-CQI | 200260 | 116572 | 42465 | 1.00 | 22079 | 20386 | 0 |
| PF | 200260 | 111386 | 47006 | 1.00 | 22829 | 24177 | 0 |

tip: load↑ 시 RR은 out↓·drop↑, Max-CQI는 thruput↑, PF는 중간
