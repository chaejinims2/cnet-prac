## OCUDU 프로젝트 보기

- [OCUDU](https://gitlab.com/ocudu/ocudu) (srsRAN 계열 OSS DU)를 라이브러리, 교본으로 본다.
- 전체 DU 빌드·3GPP 통독이 목표가 아니라, MAC 스케줄러 설정·policy·KPI가 실습의 어디에 대응하는지 이해 하는 것이 목표.
- **06–08**: 실습 주(70~~80%), OCUDU는 헤더·API 지도(20~~30%).
- **11–13**: trace·fronthaul·RRM cap 축으로 OCUDU 비중을 올려도 됨(약 40%).
- KPI 언어 맞춰보기: `jain` / `drop` / `total_out` ↔ `scheduler_metrics`, buffer·slot trace.

## Curriculum by Cursor AI

### 읽기 순서 (스케줄러만, `dev` 브랜치)

1. `[include/ocudu/scheduler/](https://gitlab.com/ocudu/ocudu/-/tree/dev/include/ocudu/scheduler)` — public API 지도
  - `[mac_scheduler.h](https://gitlab.com/ocudu/ocudu/-/blob/dev/include/ocudu/scheduler/mac_scheduler.h)` — slot / buffer / feedback 등 인터페이스 조합  
  - `[scheduler_factory.h](https://gitlab.com/ocudu/ocudu/-/blob/dev/include/ocudu/scheduler/scheduler_factory.h)` — 인스턴스 생성 진입점
2. `[scheduler_config.h](https://gitlab.com/ocudu/ocudu/-/blob/dev/include/ocudu/scheduler/config/scheduler_config.h)` — `expert_params` + `config_notifier` (실습의 `main` 파라미터에 해당)
3. `[scheduler_expert_config.h](https://gitlab.com/ocudu/ocudu/-/blob/dev/include/ocudu/scheduler/config/scheduler_expert_config.h)` — **policy·HARQ·MCS·TA** 튜닝 (알고리즘 스위치는 여기)
4. 구현(body) — GitLab/code search: `time_qos_scheduler`, `scheduler_policy`, `ue_sched` (weight·GBR·PF 계
  ### Lab ↔ OCUDU 앵커산 루프 **한 파일만** 먼저)




| Lab   | cnet-prac에서 본 것               | OCUDU에서 볼 것                                                                                                                       |
| ----- | ----------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| 02    | RR / Max-CQI / PF             | `scheduler_policy_config`: `time_rr_scheduler_config` vs `time_qos_scheduler_config`                                              |
| 04–05 | QoS weight, HOL, arrival·drop | `scheduler_dl_buffer_state_indication_handler`, DL buffer → 스케줄 입력                                                                |
| 06    | Strict priority + GBR floor   | `time_qos_scheduler_config::combine_function` — `gbr_prioritized` vs `geometric_mean`                                             |
| 07    | weight / delay / both         | `priority_enabled`, `pdb_enabled`, `gbr_enabled`, `pf_fairness_coeff`                                                             |
| 08    | Deficit RR (bytes)            | time-domain RR 그룹: `pre_policy_rr_ue_group_size` (hybrid RR+QoS 주석 참고)                                                            |
| 10    | HARQ retx (toy)               | `scheduler_ue_expert_config`: `max_nof_*_harq_retxs`, `*_harq_retx_timeout`                                                       |
| 11    | Fronthaul byte budget         | fronthaul/FAPI 쪽은 별도 모듈 — Lab 11 KPI와 slot grant clip 개념만 대조                                                                      |
| 12    | RRM cap vs MAC PF             | `pdsch_nof_rbs`, `max_pdschs_per_slot`, RB limits                                                                                 |
| 13    | `trace.csv` 재현 KPI            | `[scheduler_metrics.h](https://gitlab.com/ocudu/ocudu/-/blob/dev/include/ocudu/scheduler/scheduler_metrics.h)`, cell event tracer |




### 로컬 (선택)

```powershell
git clone --branch dev --depth 1 https://gitlab.com/ocudu/ocudu.git
# 1주차: clone + include/ocudu/scheduler 정독 + policy 구현 1~2파일
```

Lab 06 실습 노트 예: `GBR_strict` KPI 한 줄 + OCUDU `gbr_prioritized` 주석 요약을 README 또는 해당 Lab 폴더에 적어 두면 면접·복습용 연결이 유지됨.