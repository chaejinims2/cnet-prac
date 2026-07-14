#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

void reset_scenario(std::vector<UE>& ue_list, int& rr_cursor) {
    ue_list.clear();
    rr_cursor = 0;
    // 100 TTI 안에 고갈되지 않게 크게 둠 → 알고리즘별 분배 차이가 Jain에 반영됨
    ue_list.push_back(make_ue(0, 100000, 12));
    ue_list.push_back(make_ue(1, 100000, 4));
    ue_list.push_back(make_ue(2, 100000, 15));
    // 동일 seed → 알고리즘마다 같은 CQI 시퀀스
    std::srand(RNG_SEED);
}

int select_algo_custom(const std::vector<UE>& ue_list, int algo, int& rr_cursor) {
    switch (algo) {
        case ALGO_RR:
            return pick_rr(ue_list, rr_cursor);
        default:
            return select_algo(ue_list, algo);
    }
}

// 스케줄러 함수: 이번 1ms(TTI)에 어떤 단말에 자원(RB)을 줄지 결정 (algo별)
void schedule_one_tti(std::vector<UE>& ue_list, int algo, int& rr_cursor) {
    int remaining_rb = TOTAL_RB;

    int selected_index = select_algo_custom(ue_list, algo, rr_cursor);
    int allocated_data = 0;

    switch (selected_index != -1 ? 1 : 0) {
        case 1: {
            UE& selected_ue = ue_list[static_cast<size_t>(selected_index)];
            allocated_data = allocate_bytes(selected_ue);
            switch (allocated_data > remaining_rb * BYTES_PER_RB ? 1 : 0) {
                case 1:
                    allocated_data = remaining_rb * BYTES_PER_RB;
                    break;
                default:
                    break;
            }
            selected_ue.buffer_size = selected_ue.buffer_size - allocated_data;
            selected_ue.served_bytes = selected_ue.served_bytes + allocated_data;
            update_avg_rates(ue_list, selected_index, allocated_data);
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }
}

void print_summary(int algo, const std::vector<UE>& ue_list) {
    long long total = 0;
    size_t i = 0;
    std::cout << "=== " << algo_name(algo) << " ===" << std::endl;
    std::cout << "UE\tserved_bytes\tremain_buffer" << std::endl;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        total = total + ue.served_bytes;
        std::cout << ue.id << "\t" << ue.served_bytes << "\t" << ue.buffer_size << std::endl;
        i = i + 1;
    }
    std::cout << "total_bytes\t" << total << std::endl;
    std::cout << "jain_index\t" << jain_index(ue_list) << std::endl;
    std::cout << std::endl;
}

int main() {
    enable_ansi_stdout();
    std::srand(RNG_SEED);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# compare RR / Max-CQI / PF, seed=" << RNG_SEED
              << ", tti=100" << std::endl;
    std::cout << std::endl;

    int algo = 0;
    while (algo < ALGO_KIND_COUNT) {
        std::vector<UE> ue_list;
        int rr_cursor = 0;
        // 100 TTI 안에 고갈되지 않게 크게 둠 → 알고리즘별 분배 차이가 Jain에 반영됨
        ue_list.push_back(make_ue(0, 100000, 12));
        ue_list.push_back(make_ue(1, 100000, 4));
        ue_list.push_back(make_ue(2, 100000, 15));
        // 동일 seed → 알고리즘마다 같은 CQI 시퀀스

        int tti = 1;
        while (tti <= 100) {
            fade_cqi(ue_list);
            schedule_one_tti(ue_list, algo, rr_cursor);
            tti = tti + 1;
        }

        print_summary(algo, ue_list);
        algo = algo + 1;
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "compare tip: Max-CQI thruput↑ fairness↓ / RR opposite / PF middle"
              << std::endl;
    return 0;
}
