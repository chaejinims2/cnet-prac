#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

enum QosClass {
    QOS_NON_GBR = 0,
    QOS_GBR = 1
};

static const unsigned int RNG_SEED = 42;
static const int TOTAL_RB = 100;
static const int BYTES_PER_RB = 50;
static const int TTI_COUNT = 100;
static const double AVG_RATE_BIAS = 20.0;
static const double IIR_KEEP = 0.85;
static const double IIR_NEW = 0.15;

// [실습] GBR / Non-GBR 가중치 튜닝
static const double WEIGHT_NON_GBR = 1.0;
static const double WEIGHT_GBR = 5.0;

struct UE {
    int id;
    int buffer_size;
    int cqi;
    double avg_rate;
    long long served_bytes;
    int hol_delay_ms;
    int qos_class;
    int deadline_miss;
};

static std::vector<UE> g_ue_list;

static double qos_weight_of(int qos_class) {
    switch (qos_class) {
        case QOS_GBR:
            return WEIGHT_GBR;
        case QOS_NON_GBR:
            return WEIGHT_NON_GBR;
        default:
            return 1.0;
    }
}

void init() {
    g_ue_list.clear();

    UE u0;
    u0.id = 0;
    u0.buffer_size = 6000;
    u0.cqi = 10;
    u0.avg_rate = 0.0;
    u0.served_bytes = 0;
    u0.hol_delay_ms = 0;
    u0.qos_class = QOS_GBR;
    u0.deadline_miss = 0;

    UE u1;
    u1.id = 1;
    u1.buffer_size = 6000;
    u1.cqi = 10;
    u1.avg_rate = 0.0;
    u1.served_bytes = 0;
    u1.hol_delay_ms = 0;
    u1.qos_class = QOS_NON_GBR;
    u1.deadline_miss = 0;

    UE u2;
    u2.id = 2;
    u2.buffer_size = 6000;
    u2.cqi = 10;
    u2.avg_rate = 0.0;
    u2.served_bytes = 0;
    u2.hol_delay_ms = 0;
    u2.qos_class = QOS_GBR;
    u2.deadline_miss = 0;

    g_ue_list.push_back(u0);
    g_ue_list.push_back(u1);
    g_ue_list.push_back(u2);

    std::srand(RNG_SEED);
}

void input() {
}

static double delay_pf_metric(const UE& ue) {
    const double inst = static_cast<double>(ue.cqi) * static_cast<double>(BYTES_PER_RB);
    const double pf = inst / (ue.avg_rate + AVG_RATE_BIAS);
    const double w = qos_weight_of(ue.qos_class);
    // [실습] hol_delay_ms 항을 제거하면 순수 weighted-PF가 된다
    return w * static_cast<double>(ue.hol_delay_ms + 1) * pf;
}

static int pick_ue(const std::vector<UE>& ue_list) {
    int selected = -1;
    double best = -1.0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1: {
                const double m = delay_pf_metric(ue);
                switch (m > best ? 1 : 0) {
                    case 1:
                        best = m;
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

static int allocate_bytes(UE& ue) {
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

static void age_delays(std::vector<UE>& ue_list, int selected) {
    size_t i = 0;
    while (i < ue_list.size()) {
        UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1:
                switch (static_cast<int>(i) == selected ? 1 : 0) {
                    case 1:
                        // 서비스 받으면 HOL 리셋(간단 모델)
                        ue.hol_delay_ms = 0;
                        break;
                    default:
                        ue.hol_delay_ms = ue.hol_delay_ms + 1;
                        // [실습] deadline=20ms 가정. 초과 시 miss 카운트
                        switch (ue.hol_delay_ms > 20 ? 1 : 0) {
                            case 1:
                                ue.deadline_miss = ue.deadline_miss + 1;
                                break;
                            default:
                                break;
                        }
                        break;
                }
                break;
            default:
                ue.hol_delay_ms = 0;
                break;
        }
        i = i + 1;
    }
}

static void update_avg_rates(std::vector<UE>& ue_list, int selected, int allocated) {
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

static void fade_cqi(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        ue_list[i].cqi = (std::rand() % 15) + 1;
        i = i + 1;
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list, int tti_ms) {
    const int selected = pick_ue(ue_list);
    int allocated = 0;
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            allocated = allocate_bytes(ue);
            ue.buffer_size = ue.buffer_size - allocated;
            ue.served_bytes = ue.served_bytes + allocated;
            update_avg_rates(ue_list, selected, allocated);
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }

    age_delays(ue_list, selected);

    std::cout << tti_ms << "\t";
    switch (selected >= 0 ? 1 : 0) {
        case 1:
            std::cout << "UE" << selected << "\t" << allocated;
            break;
        default:
            std::cout << "NONE\t0";
            break;
    }

    size_t i = 0;
    while (i < ue_list.size()) {
        std::cout << "\tdly" << ue_list[i].hol_delay_ms;
        i = i + 1;
    }
    std::cout << std::endl;
}

void run() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# QoS delay-aware PF, seed=" << RNG_SEED << std::endl;
    std::cout << "TTI\tUE\tData\tdly0\tdly1\tdly2" << std::endl;

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(g_ue_list);
        schedule_one_tti(g_ue_list, tti);
        tti = tti + 1;
    }
}

void solution() {
    std::cout << "-------------------------------------------------" << std::endl;
    size_t i = 0;
    while (i < g_ue_list.size()) {
        const UE& ue = g_ue_list[i];
        const char* qname = "Non-GBR";
        switch (ue.qos_class) {
            case QOS_GBR:
                qname = "GBR";
                break;
            default:
                qname = "Non-GBR";
                break;
        }
        std::cout << "UE" << ue.id << " [" << qname << "] served=" << ue.served_bytes
                  << " miss=" << ue.deadline_miss << " remain=" << ue.buffer_size << std::endl;
        i = i + 1;
    }
}

int main() {
    init();
    input();
    run();
    solution();
    return 0;
}
