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
