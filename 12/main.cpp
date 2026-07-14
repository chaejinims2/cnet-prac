#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

static const int TTI_COUNT = 80;

struct SimResult {
    long long served[3];
    long long total_served;
    double jain;
};

static std::vector<UE> make_ue_list(int cap_on) {
    std::vector<UE> ue_list;
    UE u0 = make_ue(0, 20000, 12);
    UE u1 = make_ue(1, 20000, 10);
    UE u2 = make_ue(2, 20000, 15);
    switch (cap_on) {
        case 1:
            u0.max_bytes_per_tti = 300;
            u1.max_bytes_per_tti = 450;
            u2.max_bytes_per_tti = 600;
            break;
        default:
            u0.max_bytes_per_tti = 0;
            u1.max_bytes_per_tti = 0;
            u2.max_bytes_per_tti = 0;
            break;
    }
    ue_list.push_back(u0);
    ue_list.push_back(u1);
    ue_list.push_back(u2);
    return ue_list;
}

static void schedule_one_tti(std::vector<UE>& ue_list) {
    const int selected = pick_pf(ue_list);
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            int allocated = allocate_bytes(ue);
            switch (ue.max_bytes_per_tti > 0 ? 1 : 0) {
                case 1:
                    switch (allocated > ue.max_bytes_per_tti ? 1 : 0) {
                        case 1:
                            allocated = ue.max_bytes_per_tti;
                            break;
                        default:
                            break;
                    }
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
            ue.buffer_size = ue.buffer_size - allocated;
            ue.served_bytes = ue.served_bytes + allocated;
            update_avg_rates(ue_list, selected, allocated);
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }
}

static void run_mode(int cap_on, SimResult& out) {
    std::vector<UE> ue_list = make_ue_list(cap_on);
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_one_tti(ue_list);
        tti = tti + 1;
    }

    out.total_served = 0;
    size_t i = 0;
    while (i < ue_list.size()) {
        out.served[i] = ue_list[i].served_bytes;
        out.total_served = out.total_served + ue_list[i].served_bytes;
        i = i + 1;
    }
    out.jain = jain_index(ue_list);
}

int main() {
    enable_ansi_stdout();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# RRM max bitrate cap + MAC PF, seed=" << RNG_SEED << std::endl;
    std::cout << std::endl;

    SimResult capped;
    SimResult free_run;
    run_mode(1, capped);
    run_mode(0, free_run);

    std::cout << "=== RRM_cap ===" << std::endl;
    std::cout << "UE0=" << capped.served[0] << " UE1=" << capped.served[1]
              << " UE2=" << capped.served[2] << " jain=" << capped.jain << std::endl;
    std::cout << "=== no_cap ===" << std::endl;
    std::cout << "UE0=" << free_run.served[0] << " UE1=" << free_run.served[1]
              << " UE2=" << free_run.served[2] << " jain=" << free_run.jain << std::endl;
    std::cout << std::endl;

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\tue0_srv\tue1_srv\tue2_srv\ttotal\tjain" << std::endl;
    std::cout << "RRM_cap\t" << capped.served[0] << "\t" << capped.served[1] << "\t"
              << capped.served[2] << "\t" << capped.total_served << "\t" << capped.jain
              << std::endl;
    std::cout << "no_cap\t" << free_run.served[0] << "\t" << free_run.served[1] << "\t"
              << free_run.served[2] << "\t" << free_run.total_served << "\t" << free_run.jain
              << std::endl;
    return 0;
}
