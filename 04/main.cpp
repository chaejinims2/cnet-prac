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

static const double WEIGHT_NON_GBR = 1.0;
static const double WEIGHT_GBR = 5.0;

struct ModeResult {
    long long served[3];
    int miss[3];
    int picks[3];
};

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

static std::vector<UE> make_ue_list() {
    std::vector<UE> ue_list;
    UE u0 = make_ue(0, 6000, 10);
    u0.qos_class = QOS_GBR;
    UE u1 = make_ue(1, 6000, 10);
    u1.qos_class = QOS_NON_GBR;
    UE u2 = make_ue(2, 6000, 10);
    u2.qos_class = QOS_GBR;
    ue_list.push_back(u0);
    ue_list.push_back(u1);
    ue_list.push_back(u2);
    return ue_list;
}

static double delay_pf_metric(const UE& ue, int use_delay) {
    const double pf = pf_metric(ue);
    const double w = qos_weight_of(ue.qos_class);
    switch (use_delay) {
        case 1:
            return w * static_cast<double>(ue.hol_delay_ms + 1) * pf;
        default:
            return w * pf;
    }
}

static int pick_ue(const std::vector<UE>& ue_list, int use_delay) {
    int selected = -1;
    double best = -1.0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1: {
                const double m = delay_pf_metric(ue, use_delay);
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

static void schedule_one_tti(std::vector<UE>& ue_list, int use_delay, int pick_count[3]) {
    const int selected = pick_ue(ue_list, use_delay);
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            const int allocated = allocate_bytes(ue);
            ue.buffer_size = ue.buffer_size - allocated;
            ue.served_bytes = ue.served_bytes + allocated;
            update_avg_rates(ue_list, selected, allocated);
            pick_count[selected] = pick_count[selected] + 1;
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

static void fill_result(const std::vector<UE>& ue_list, const int pick_count[3],
                        ModeResult& out) {
    size_t i = 0;
    while (i < ue_list.size()) {
        out.served[i] = ue_list[i].served_bytes;
        out.miss[i] = ue_list[i].deadline_miss;
        out.picks[i] = pick_count[i];
        i = i + 1;
    }
}

static void print_mode_summary(int use_delay, const std::vector<UE>& ue_list,
                             const ModeResult& r) {
    std::cout << "=== " << mode_name(use_delay) << " ===" << std::endl;
    size_t i = 0;
    while (i < ue_list.size()) {
        const char* qname = "Non-GBR";
        switch (ue_list[i].qos_class) {
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
    std::vector<UE> ue_list = make_ue_list();
    int pick_count[3] = {0, 0, 0};
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_one_tti(ue_list, use_delay, pick_count);
        tti = tti + 1;
    }

    fill_result(ue_list, pick_count, out);
    print_mode_summary(use_delay, ue_list, out);
}

int main() {
    enable_ansi_stdout();

    ModeResult res_on;
    ModeResult res_off;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# QoS delay ON/OFF compare, seed=" << RNG_SEED
              << ", tti=" << TTI_COUNT << std::endl;
    std::cout << std::endl;

    run_one_mode(1, res_on);
    run_one_mode(0, res_off);

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\tue0_srv\tue1_srv\tue2_srv\tue0_miss\tue1_miss\tue2_miss"
              << "\tue0_picks\tue1_picks\tue2_picks" << std::endl;
    std::cout << "delay_ON\t"
              << res_on.served[0] << "\t" << res_on.served[1] << "\t" << res_on.served[2]
              << "\t" << res_on.miss[0] << "\t" << res_on.miss[1] << "\t" << res_on.miss[2]
              << "\t" << res_on.picks[0] << "\t" << res_on.picks[1] << "\t" << res_on.picks[2]
              << std::endl;
    std::cout << "delay_OFF\t"
              << res_off.served[0] << "\t" << res_off.served[1] << "\t" << res_off.served[2]
              << "\t" << res_off.miss[0] << "\t" << res_off.miss[1] << "\t" << res_off.miss[2]
              << "\t" << res_off.picks[0] << "\t" << res_off.picks[1] << "\t" << res_off.picks[2]
              << std::endl;
    std::cout << "compare tip: delay ON → HOL 급한 UE 우선 / OFF → Non-GBR(UE1) picks 변화 관찰"
              << std::endl;
    return 0;
}
