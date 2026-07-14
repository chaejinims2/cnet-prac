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
static const double WEIGHT_GBR = 5.0;
static const double WEIGHT_NON_GBR = 1.0;

struct ModeResult {
    long long served[3];
    int picks[3];
};

static double qos_weight(int qos_class) {
    switch (qos_class) {
        case QOS_GBR:
            return WEIGHT_GBR;
        default:
            return WEIGHT_NON_GBR;
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

static double metric_of(const UE& ue, int mode) {
    const double pf = pf_metric(ue);
    const double w = qos_weight(ue.qos_class);
    switch (mode) {
        case 0:
            return w * pf;
        case 1:
            return static_cast<double>(ue.hol_delay_ms + 1) * pf;
        default:
            return w * static_cast<double>(ue.hol_delay_ms + 1) * pf;
    }
}

static int pick_ue(const std::vector<UE>& ue_list, int mode) {
    int selected = -1;
    double best = -1.0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1: {
                const double m = metric_of(ue, mode);
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

static void schedule_one_tti(std::vector<UE>& ue_list, int mode, int picks[3]) {
    const int selected = pick_ue(ue_list, mode);
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            const int allocated = allocate_bytes(ue);
            ue.buffer_size = ue.buffer_size - allocated;
            ue.served_bytes = ue.served_bytes + allocated;
            update_avg_rates(ue_list, selected, allocated);
            picks[selected] = picks[selected] + 1;
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }
    age_delays(ue_list, selected);
}

static const char* mode_name(int mode) {
    switch (mode) {
        case 0:
            return "weight_only";
        case 1:
            return "delay_only";
        default:
            return "both";
    }
}

static void run_mode(int mode, ModeResult& out) {
    std::vector<UE> ue_list = make_ue_list();
    int picks[3];
    picks[0] = 0;
    picks[1] = 0;
    picks[2] = 0;
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_one_tti(ue_list, mode, picks);
        tti = tti + 1;
    }

    size_t i = 0;
    while (i < ue_list.size()) {
        out.served[i] = ue_list[i].served_bytes;
        out.picks[i] = picks[i];
        i = i + 1;
    }
}

int main() {
    enable_ansi_stdout();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# Weighted PF / M-LWDF / both compare, seed=" << RNG_SEED << std::endl;
    std::cout << std::endl;

    ModeResult res[3];
    int mode = 0;
    while (mode < 3) {
        run_mode(mode, res[mode]);
        std::cout << "=== " << mode_name(mode) << " ===" << std::endl;
        size_t i = 0;
        while (i < 3) {
            std::cout << "UE" << i << " served=" << res[mode].served[i]
                      << " picks=" << res[mode].picks[i] << std::endl;
            i = i + 1;
        }
        std::cout << std::endl;
        mode = mode + 1;
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\tue0_picks\tue1_picks\tue2_picks\tue1_served" << std::endl;
    mode = 0;
    while (mode < 3) {
        std::cout << mode_name(mode) << "\t" << res[mode].picks[0] << "\t" << res[mode].picks[1]
                  << "\t" << res[mode].picks[2] << "\t" << res[mode].served[1] << std::endl;
        mode = mode + 1;
    }
    return 0;
}
