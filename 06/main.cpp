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

struct RunResult {
    long long served[3];
    int gbr_met[3];
    int picks[3];
};

static std::vector<UE> make_ue_list() {
    std::vector<UE> ue_list;
    UE u0 = make_ue(0, 8000, 10);
    u0.qos_class = QOS_GBR;
    u0.gbr_floor = 4500;
    UE u1 = make_ue(1, 8000, 10);
    u1.qos_class = QOS_NON_GBR;
    UE u2 = make_ue(2, 8000, 10);
    u2.qos_class = QOS_GBR;
    u2.gbr_floor = 4500;
    ue_list.push_back(u0);
    ue_list.push_back(u1);
    ue_list.push_back(u2);
    return ue_list;
}

static int pick_gbr_strict(const std::vector<UE>& ue_list) {
    int selected = -1;
    double worst_ratio = 2.0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch ((ue.buffer_size > 0 && ue.gbr_floor > 0 && ue.served_bytes < ue.gbr_floor) ? 1 : 0) {
            case 1: {
                const double ratio =
                    static_cast<double>(ue.served_bytes) / static_cast<double>(ue.gbr_floor);
                switch (ratio < worst_ratio ? 1 : 0) {
                    case 1:
                        worst_ratio = ratio;
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
    switch (selected >= 0 ? 1 : 0) {
        case 1:
            return selected;
        default:
            return pick_pf(ue_list);
    }
}

static int pick_mode(const std::vector<UE>& ue_list, int mode) {
    switch (mode) {
        case 0:
            return pick_gbr_strict(ue_list);
        default:
            return pick_pf(ue_list);
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list, int mode, int picks[3]) {
    const int selected = pick_mode(ue_list, mode);
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
}

static void run_mode(int mode, RunResult& out) {
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
        switch (ue_list[i].gbr_floor > 0 ? 1 : 0) {
            case 1:
                switch (ue_list[i].served_bytes >= ue_list[i].gbr_floor ? 1 : 0) {
                    case 1:
                        out.gbr_met[i] = 1;
                        break;
                    default:
                        out.gbr_met[i] = 0;
                        break;
                }
                break;
            default:
                out.gbr_met[i] = -1;
                break;
        }
        i = i + 1;
    }
}

static const char* mode_name(int mode) {
    switch (mode) {
        case 0:
            return "GBR_strict";
        default:
            return "PF_only";
    }
}

int main() {
    enable_ansi_stdout();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# Strict Priority + GBR floor vs PF, seed=" << RNG_SEED << std::endl;
    std::cout << std::endl;

    RunResult r0;
    RunResult r1;
    run_mode(0, r0);
    run_mode(1, r1);

    int mode = 0;
    while (mode < 2) {
        RunResult* r = 0;
        switch (mode) {
            case 0:
                r = &r0;
                break;
            default:
                r = &r1;
                break;
        }
        std::cout << "=== " << mode_name(mode) << " ===" << std::endl;
        size_t i = 0;
        while (i < 3) {
            std::cout << "UE" << i << " served=" << r->served[i]
                      << " picks=" << r->picks[i]
                      << " gbr_met=" << r->gbr_met[i] << std::endl;
            i = i + 1;
        }
        std::cout << std::endl;
        mode = mode + 1;
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\tue0_srv\tue1_srv\tue2_srv\tue0_gbr\tue2_gbr\tue1_picks" << std::endl;
    std::cout << "GBR_strict\t" << r0.served[0] << "\t" << r0.served[1] << "\t" << r0.served[2]
              << "\t" << r0.gbr_met[0] << "\t" << r0.gbr_met[2] << "\t" << r0.picks[1]
              << std::endl;
    std::cout << "PF_only\t" << r1.served[0] << "\t" << r1.served[1] << "\t" << r1.served[2]
              << "\t" << r1.gbr_met[0] << "\t" << r1.gbr_met[2] << "\t" << r1.picks[1]
              << std::endl;
    return 0;
}
