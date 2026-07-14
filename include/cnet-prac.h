#pragma once

#include <cstdlib>
#include <vector>

// 공통 무선 자원 / PF 파라미터 (실습 01~05)
enum {
    TOTAL_RB = 100,
    BYTES_PER_RB = 50
};

static const unsigned int RNG_SEED = 42;
static const double AVG_RATE_BIAS = 20.0;
static const double IIR_KEEP = 0.85;
static const double IIR_NEW = 0.15;

// UE: 실습별 확장 필드를 한 구조체에 모음 (안 쓰는 필드는 0 유지)
struct UE {
    int id;
    int buffer_size;
    int cqi;
    double weight;
    double avg_rate;
    long long served_bytes;
    int allocated_this_tti;
    int hol_delay_ms;
    int qos_class;
    int deadline_miss;
    long long arrived_bytes;
    long long dropped_bytes;
    int arrival_mean;
};

inline UE make_ue(int id, int buffer_size, int cqi) {
    UE ue;
    ue.id = id;
    ue.buffer_size = buffer_size;
    ue.cqi = cqi;
    ue.weight = 1.0;
    ue.avg_rate = 0.0;
    ue.served_bytes = 0;
    ue.allocated_this_tti = 0;
    ue.hol_delay_ms = 0;
    ue.qos_class = 0;
    ue.deadline_miss = 0;
    ue.arrived_bytes = 0;
    ue.dropped_bytes = 0;
    ue.arrival_mean = 0;
    return ue;
}

inline double pf_inst_rate(const UE& ue) {
    return static_cast<double>(ue.cqi) * static_cast<double>(BYTES_PER_RB);
}

inline double pf_metric(const UE& ue) {
    return pf_inst_rate(ue) / (ue.avg_rate + AVG_RATE_BIAS);
}

inline int allocate_bytes(const UE& ue) {
    int allocated = ue.cqi * BYTES_PER_RB;
    const int rb_budget_bytes = TOTAL_RB * BYTES_PER_RB;
    switch (allocated > rb_budget_bytes ? 1 : 0) {
        case 1:
            allocated = rb_budget_bytes;
            break;
        default:
            break;
    }
    switch (allocated > ue.buffer_size ? 1 : 0) {
        case 1:
            allocated = ue.buffer_size;
            break;
        default:
            break;
    }
    return allocated;
}

inline void fade_cqi(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        ue_list[i].cqi = (std::rand() % 15) + 1;
        i = i + 1;
    }
}

inline void update_avg_rates(std::vector<UE>& ue_list, int selected, int allocated) {
    size_t i = 0;
    while (i < ue_list.size()) {
        UE& ue = ue_list[i];
        switch (static_cast<int>(i) == selected ? 1 : 0) {
            case 1:
                ue.avg_rate = IIR_KEEP * ue.avg_rate + IIR_NEW * static_cast<double>(allocated);
                break;
            default:
                ue.avg_rate = IIR_KEEP * ue.avg_rate;
                break;
        }
        i = i + 1;
    }
}

inline void update_avg_rates_multi(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        UE& ue = ue_list[i];
        ue.avg_rate =
            IIR_KEEP * ue.avg_rate + IIR_NEW * static_cast<double>(ue.allocated_this_tti);
        i = i + 1;
    }
}

inline int pick_pf(const std::vector<UE>& ue_list) {
    int selected = -1;
    double max_metric = -1.0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1: {
                const double metric = pf_metric(ue);
                switch (metric > max_metric ? 1 : 0) {
                    case 1:
                        max_metric = metric;
                        selected = static_cast<int>(i);
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
        i = i + 1;
    }
    return selected;
}

inline double jain_index(const std::vector<UE>& ue_list) {
    double sum = 0.0;
    double sum_sq = 0.0;
    const double n = static_cast<double>(ue_list.size());
    size_t i = 0;
    while (i < ue_list.size()) {
        const double r = static_cast<double>(ue_list[i].served_bytes);
        sum = sum + r;
        sum_sq = sum_sq + r * r;
        i = i + 1;
    }
    switch (sum_sq <= 0.0 ? 1 : 0) {
        case 1:
            return 0.0;
        default:
            return (sum * sum) / (n * sum_sq);
    }
}
