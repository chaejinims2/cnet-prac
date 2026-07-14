#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

static const int TTI_COUNT = 100;
static const int NACK_PERIOD = 5;

struct SimResult {
    long long served[3];
    int max_queue[3];
    long long total_retx;
    int harq_sched_count;
};

static std::vector<UE> make_ue_list() {
    std::vector<UE> ue_list;
    ue_list.push_back(make_ue(0, 6000, 10));
    ue_list.push_back(make_ue(1, 6000, 10));
    ue_list.push_back(make_ue(2, 6000, 10));
    return ue_list;
}

static int pick_harq_first(const std::vector<UE>& ue_list) {
    int selected = -1;
    int best_retx = 0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.harq_retx_bytes > best_retx ? 1 : 0) {
            case 1:
                best_retx = ue.harq_retx_bytes;
                selected = static_cast<int>(i);
                break;
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

static int pick_mode(const std::vector<UE>& ue_list, int harq_on) {
    switch (harq_on) {
        case 1:
            return pick_harq_first(ue_list);
        default:
            return pick_pf(ue_list);
    }
}

static void maybe_nack(UE& ue, int allocated, int tti, int harq_on, long long& total_retx) {
    switch (harq_on) {
        case 0:
            return;
        default:
            break;
    }
    switch (allocated <= 0 ? 1 : 0) {
        case 1:
            return;
        default:
            break;
    }
    switch ((tti + ue.id) % NACK_PERIOD == 0 ? 1 : 0) {
        case 1:
            ue.harq_retx_bytes = ue.harq_retx_bytes + allocated / 2;
            total_retx = total_retx + allocated / 2;
            break;
        default:
            break;
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list, int harq_on, int tti, SimResult& acc) {
    const int selected = pick_mode(ue_list, harq_on);
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            switch (ue.harq_retx_bytes > 0 ? 1 : 0) {
                case 1:
                    acc.harq_sched_count = acc.harq_sched_count + 1;
                    break;
                default:
                    break;
            }
            int allocated = allocate_bytes(ue);
            switch (ue.harq_retx_bytes > 0 && allocated > ue.harq_retx_bytes ? 1 : 0) {
                case 1:
                    allocated = ue.harq_retx_bytes;
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
            switch (ue.harq_retx_bytes > 0 ? 1 : 0) {
                case 1:
                    switch (allocated >= ue.harq_retx_bytes ? 1 : 0) {
                        case 1:
                            ue.harq_retx_bytes = 0;
                            break;
                        default:
                            ue.harq_retx_bytes = ue.harq_retx_bytes - allocated;
                            break;
                    }
                    break;
                default:
                    ue.buffer_size = ue.buffer_size - allocated;
                    maybe_nack(ue, allocated, tti, harq_on, acc.total_retx);
                    break;
            }
            ue.served_bytes = ue.served_bytes + allocated;
            update_avg_rates(ue_list, selected, allocated);
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }

    size_t i = 0;
    while (i < ue_list.size()) {
        const int q = ue_list[i].buffer_size + ue_list[i].harq_retx_bytes;
        switch (q > acc.max_queue[i] ? 1 : 0) {
            case 1:
                acc.max_queue[i] = q;
                break;
            default:
                break;
        }
        i = i + 1;
    }
}

static void run_sim(int harq_on, SimResult& out) {
    std::vector<UE> ue_list = make_ue_list();
    out.total_retx = 0;
    out.harq_sched_count = 0;
    out.max_queue[0] = 0;
    out.max_queue[1] = 0;
    out.max_queue[2] = 0;
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_one_tti(ue_list, harq_on, tti, out);
        tti = tti + 1;
    }

    size_t i = 0;
    while (i < ue_list.size()) {
        out.served[i] = ue_list[i].served_bytes;
        i = i + 1;
    }
}

int main() {
    enable_ansi_stdout();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# HARQ retx buffer pressure, seed=" << RNG_SEED << std::endl;
    std::cout << std::endl;

    SimResult on;
    SimResult off;
    run_sim(1, on);
    run_sim(0, off);

    std::cout << "=== harq_ON ===" << std::endl;
    std::cout << "total_retx=" << on.total_retx << " harq_sched=" << on.harq_sched_count
              << " max_q0=" << on.max_queue[0] << std::endl;
    std::cout << std::endl;
    std::cout << "=== harq_OFF ===" << std::endl;
    std::cout << "total_retx=" << off.total_retx << " max_q0=" << off.max_queue[0] << std::endl;
    std::cout << std::endl;

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\ttotal_retx\tharq_sched\tmax_q_sum\ttotal_served" << std::endl;
    std::cout << "harq_ON\t" << on.total_retx << "\t" << on.harq_sched_count << "\t"
              << (on.max_queue[0] + on.max_queue[1] + on.max_queue[2]) << "\t"
              << (on.served[0] + on.served[1] + on.served[2]) << std::endl;
    std::cout << "harq_OFF\t" << off.total_retx << "\t0\t"
              << (off.max_queue[0] + off.max_queue[1] + off.max_queue[2]) << "\t"
              << (off.served[0] + off.served[1] + off.served[2]) << std::endl;
    return 0;
}
