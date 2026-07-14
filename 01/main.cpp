#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

// 스케줄러 함수: 이번 1ms(TTI)에 어떤 단말에 자원(RB)을 줄지 결정
void schedule_one_tti(std::vector<UE>& ue_list, int algo, int tti_ms) {
    int remaining_rb = TOTAL_RB;

    int selected_index = select_algo(ue_list, algo);
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
            update_avg_rates(ue_list, selected_index, allocated_data);
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }

    switch (selected_index != -1 ? 1 : 0) {
        case 1: {
            const UE& selected_ue = ue_list[static_cast<size_t>(selected_index)];
            std::cout << tti_ms << "\t" << selected_ue.id << "\t" << allocated_data;
            size_t i = 0;
            while (i < ue_list.size()) {
                const double r = ue_list[i].avg_rate;
                std::cout << "\t";
                switch (static_cast<int>(i) == selected_index ? 1 : 0) {
                    case 1:
                        // std::cout << kAnsiHighlight;
                        break;
                    default:
                        break;
                }
                std::cout << r << "(" << ue_list[i].cqi << ")";
                switch (static_cast<int>(i) == selected_index ? 1 : 0) {
                    case 1:
                        // std::cout << kAnsiReset;
                        break;
                    default:
                        break;
                }
                i = i + 1;
            }
            std::cout << std::endl;
            break;
        }
        default:
            break;
    }
}

int main() {
    enable_ansi_stdout();
    std::srand(RNG_SEED);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "TTI(ms)" << "\t" << "UE(id)" << "\t" << "Data(Bytes)"
              << "\t" << "avg_R0(cqi)" << "\t" << "avg_R1(cqi)" << "\t" << "avg_R2(cqi)"
              << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;

    int tti = 1;
    int algo = ALGO_PF;
    while (algo < ALGO_KIND_COUNT) {

        std::vector<UE> ue_list;
        ue_list.push_back(make_ue(0, 5000, 12));
        ue_list.push_back(make_ue(1, 5000, 4));
        ue_list.push_back(make_ue(2, 1000, 15));
        while (tti <= 100) {
            fade_cqi(ue_list);
            schedule_one_tti(ue_list, algo, tti);
            tti = tti + 1;
        }
        algo = algo + 1;
    }

    return 0;
}
