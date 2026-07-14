#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

enum QosClass {
    QOS_NON_GBR = 0,
    QOS_GBR = 1
};

static const int TTI_COUNT = 100;

// [실습] GBR / Non-GBR 가중치 튜닝
static const double WEIGHT_NON_GBR = 1.0;
static const double WEIGHT_GBR = 5.0;

struct ModeResult {
    long long served[3];
    int miss[3];
    int picks[3];
};

static std::vector<UE> g_ue_list;
static int g_use_delay = 1;
static int g_pick_count[3];
static ModeResult g_res_on;
static ModeResult g_res_off;

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

    UE u0 = make_ue(0, 6000, 10);
    u0.qos_class = QOS_GBR;
    UE u1 = make_ue(1, 6000, 10);
    u1.qos_class = QOS_NON_GBR;
    UE u2 = make_ue(2, 6000, 10);
    u2.qos_class = QOS_GBR;

    g_ue_list.push_back(u0);
    g_ue_list.push_back(u1);
    g_ue_list.push_back(u2);

    g_pick_count[0] = 0;
    g_pick_count[1] = 0;
    g_pick_count[2] = 0;
    std::srand(RNG_SEED);
}

void input() {
}

static double delay_pf_metric(const UE& ue) {
    const double pf = pf_metric(ue);
    const double w = qos_weight_of(ue.qos_class);
    // [실습] delay OFF 이면 hol 항 제거 → 순수 weighted-PF
    switch (g_use_delay) {
        case 1:
            return w * static_cast<double>(ue.hol_delay_ms + 1) * pf;
        default:
            return w * pf;
    }
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

static void age_delays(std::vector<UE>& ue_list, int selected) {
    size_t i = 0;
    while (i < ue_list.size()) {
        UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1:
                switch (static_cast<int>(i) == selected ? 1 : 0) {
                    case 1:
                        ue.hol_delay_ms = 0;
                        break;
                    default:
                        ue.hol_delay_ms = ue.hol_delay_ms + 1;
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

static void schedule_one_tti(std::vector<UE>& ue_list) {
    const int selected = pick_ue(ue_list);
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            const int allocated = allocate_bytes(ue);
            ue.buffer_size = ue.buffer_size - allocated;
            ue.served_bytes = ue.served_bytes + allocated;
            update_avg_rates(ue_list, selected, allocated);
            g_pick_count[selected] = g_pick_count[selected] + 1;
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }
    age_delays(ue_list, selected);
}

static const char* mode_name(int use_delay) {
    switch (use_delay) {
        case 1:
            return "delay_ON";
        default:
            return "delay_OFF";
    }
}

static void save_result(ModeResult& out) {
    size_t i = 0;
    while (i < g_ue_list.size()) {
        out.served[i] = g_ue_list[i].served_bytes;
        out.miss[i] = g_ue_list[i].deadline_miss;
        out.picks[i] = g_pick_count[i];
        i = i + 1;
    }
}

static void print_mode_summary(int use_delay, const ModeResult& r) {
    std::cout << "=== " << mode_name(use_delay) << " ===" << std::endl;
    size_t i = 0;
    while (i < g_ue_list.size()) {
        const char* qname = "Non-GBR";
        switch (g_ue_list[i].qos_class) {
            case QOS_GBR:
                qname = "GBR";
                break;
            default:
                qname = "Non-GBR";
                break;
        }
        std::cout << "UE" << i << " [" << qname << "] served=" << r.served[i]
                  << " miss=" << r.miss[i]
                  << " picks=" << r.picks[i] << std::endl;
        i = i + 1;
    }
    std::cout << std::endl;
}

static void run_one_mode(int use_delay, ModeResult& out) {
    g_use_delay = use_delay;
    init();

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(g_ue_list);
        schedule_one_tti(g_ue_list);
        tti = tti + 1;
    }

    save_result(out);
    print_mode_summary(use_delay, out);
}

void run() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# QoS delay ON/OFF compare, seed=" << RNG_SEED
              << ", tti=" << TTI_COUNT << std::endl;
    std::cout << std::endl;

    run_one_mode(1, g_res_on);
    run_one_mode(0, g_res_off);
}

void solution() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\tue0_srv\tue1_srv\tue2_srv\tue0_miss\tue1_miss\tue2_miss"
              << "\tue0_picks\tue1_picks\tue2_picks" << std::endl;
    std::cout << "delay_ON\t"
              << g_res_on.served[0] << "\t" << g_res_on.served[1] << "\t" << g_res_on.served[2]
              << "\t" << g_res_on.miss[0] << "\t" << g_res_on.miss[1] << "\t" << g_res_on.miss[2]
              << "\t" << g_res_on.picks[0] << "\t" << g_res_on.picks[1] << "\t" << g_res_on.picks[2]
              << std::endl;
    std::cout << "delay_OFF\t"
              << g_res_off.served[0] << "\t" << g_res_off.served[1] << "\t" << g_res_off.served[2]
              << "\t" << g_res_off.miss[0] << "\t" << g_res_off.miss[1] << "\t" << g_res_off.miss[2]
              << "\t" << g_res_off.picks[0] << "\t" << g_res_off.picks[1] << "\t" << g_res_off.picks[2]
              << std::endl;
    std::cout << "compare tip: delay ON → HOL 급한 UE 우선 / OFF → Non-GBR(UE1) picks 변화 관찰"
              << std::endl;
}

int main() {
    enable_ansi_stdout();
    init();
    input();
    run();
    solution();
    return 0;
}
