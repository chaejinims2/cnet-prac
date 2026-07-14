#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"

static const int TTI_COUNT = 200;

// [실습] 유입량 / 버퍼 상한 튜닝
static const int ARRIVAL_MEAN_UE0 = 400;
static const int ARRIVAL_MEAN_UE1 = 400;
static const int ARRIVAL_MEAN_UE2 = 200;
static const int ARRIVAL_JITTER = 100;
static const int BUFFER_CAP = 20000;

static std::vector<UE> g_ue_list;

void init() {
    g_ue_list.clear();

    UE u0 = make_ue(0, 0, 10);
    u0.arrival_mean = ARRIVAL_MEAN_UE0;
    UE u1 = make_ue(1, 0, 10);
    u1.arrival_mean = ARRIVAL_MEAN_UE1;
    UE u2 = make_ue(2, 0, 10);
    u2.arrival_mean = ARRIVAL_MEAN_UE2;

    g_ue_list.push_back(u0);
    g_ue_list.push_back(u1);
    g_ue_list.push_back(u2);
    std::srand(RNG_SEED);
}

void input() {
}

static int sample_arrival(int mean) {
    int half = ARRIVAL_JITTER / 2;
    int jitter = 0;
    switch (ARRIVAL_JITTER > 0 ? 1 : 0) {
        case 1:
            jitter = (std::rand() % ARRIVAL_JITTER) - half;
            break;
        default:
            jitter = 0;
            break;
    }
    int arrival = mean + jitter;
    switch (arrival < 0 ? 1 : 0) {
        case 1:
            arrival = 0;
            break;
        default:
            break;
    }
    return arrival;
}

static void arrive_traffic(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        UE& ue = ue_list[i];
        const int arrival = sample_arrival(ue.arrival_mean);
        ue.arrived_bytes = ue.arrived_bytes + arrival;

        int next_buf = ue.buffer_size + arrival;
        switch (next_buf > BUFFER_CAP ? 1 : 0) {
            case 1: {
                const int accepted = BUFFER_CAP - ue.buffer_size;
                switch (accepted > 0 ? 1 : 0) {
                    case 1:
                        ue.buffer_size = BUFFER_CAP;
                        ue.dropped_bytes = ue.dropped_bytes + (arrival - accepted);
                        break;
                    default:
                        ue.dropped_bytes = ue.dropped_bytes + arrival;
                        break;
                }
                break;
            }
            default:
                ue.buffer_size = next_buf;
                break;
        }
        i = i + 1;
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list, int tti_ms) {
    const int selected = pick_pf(ue_list);
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

    switch ((tti_ms % 20) == 0 ? 1 : 0) {
        case 1: {
            std::cout << tti_ms << "\tUE" << selected << "\t" << allocated;
            size_t i = 0;
            while (i < ue_list.size()) {
                std::cout << "\tq" << ue_list[i].id << "=" << ue_list[i].buffer_size;
                i = i + 1;
            }
            std::cout << std::endl;
            break;
        }
        default:
            break;
    }
}

void run() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# traffic arrival + PF, seed=" << RNG_SEED << std::endl;
    std::cout << "TTI\tUE\tData\tqueue_snapshot" << std::endl;

    int tti = 1;
    while (tti <= TTI_COUNT) {
        arrive_traffic(g_ue_list);
        fade_cqi(g_ue_list);
        schedule_one_tti(g_ue_list, tti);
        tti = tti + 1;
    }
}

void solution() {
    std::cout << "-------------------------------------------------" << std::endl;
    long long sum_in = 0;
    long long sum_out = 0;
    long long sum_drop = 0;
    size_t i = 0;
    while (i < g_ue_list.size()) {
        const UE& ue = g_ue_list[i];
        sum_in = sum_in + ue.arrived_bytes;
        sum_out = sum_out + ue.served_bytes;
        sum_drop = sum_drop + ue.dropped_bytes;
        std::cout << "UE" << ue.id
                  << " in=" << ue.arrived_bytes
                  << " out=" << ue.served_bytes
                  << " drop=" << ue.dropped_bytes
                  << " q=" << ue.buffer_size << std::endl;
        i = i + 1;
    }
    std::cout << "TOTAL in=" << sum_in << " out=" << sum_out << " drop=" << sum_drop
              << std::endl;
}

int main() {
    init();
    input();
    run();
    solution();
    return 0;
}
