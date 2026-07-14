#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

static const int TTI_COUNT = 100;
static const int FRONT_BUDGET = 450;

struct SimResult {
    long long served[3];
    long long total_served;
    int budget_exceed_tti;
    int avg_delay_penalty;
};

static std::vector<UE> make_ue_list() {
    std::vector<UE> ue_list;
    ue_list.push_back(make_ue(0, 12000, 12));
    ue_list.push_back(make_ue(1, 12000, 10));
    ue_list.push_back(make_ue(2, 12000, 15));
    return ue_list;
}

static void schedule_one_tti(std::vector<UE>& ue_list, int budget_on, int& exceed, int& delay_penalty) {
    const int selected = pick_pf(ue_list);
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            int allocated = allocate_bytes(ue);
            switch (budget_on) {
                case 1:
                    switch (allocated > FRONT_BUDGET ? 1 : 0) {
                        case 1:
                            delay_penalty = delay_penalty + (allocated - FRONT_BUDGET);
                            allocated = FRONT_BUDGET;
                            exceed = exceed + 1;
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

static void run_mode(int budget_on, SimResult& out) {
    std::vector<UE> ue_list = make_ue_list();
    int exceed = 0;
    int delay_penalty = 0;
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_one_tti(ue_list, budget_on, exceed, delay_penalty);
        tti = tti + 1;
    }

    out.total_served = 0;
    size_t i = 0;
    while (i < ue_list.size()) {
        out.served[i] = ue_list[i].served_bytes;
        out.total_served = out.total_served + ue_list[i].served_bytes;
        i = i + 1;
    }
    out.budget_exceed_tti = exceed;
    out.avg_delay_penalty = delay_penalty / TTI_COUNT;
}

int main() {
    enable_ansi_stdout();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# fronthaul byte budget=" << FRONT_BUDGET << ", seed=" << RNG_SEED << std::endl;
    std::cout << std::endl;

    SimResult limited;
    SimResult unlimited;
    run_mode(1, limited);
    run_mode(0, unlimited);

    std::cout << "=== budget_ON ===" << std::endl;
    std::cout << "total_served=" << limited.total_served
              << " clipped_tti=" << limited.budget_exceed_tti << std::endl;
    std::cout << "=== budget_OFF ===" << std::endl;
    std::cout << "total_served=" << unlimited.total_served << std::endl;
    std::cout << std::endl;

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\ttotal_served\tclipped_tti\tpenalty_avg" << std::endl;
    std::cout << "budget_ON\t" << limited.total_served << "\t" << limited.budget_exceed_tti
              << "\t" << limited.avg_delay_penalty << std::endl;
    std::cout << "budget_OFF\t" << unlimited.total_served << "\t0\t0" << std::endl;
    return 0;
}
