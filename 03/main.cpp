#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

static const int TTI_COUNT = 50;

static bool metric_greater(const Candidate& a, const Candidate& b) {
    return a.metric > b.metric;
}

// [실습] 핵심: remaining_rb 를 여러 UE에 나눠 주기
void allocate_rbs_multi(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        ue_list[i].allocated_this_tti = 0;
        i = i + 1;
    }

    std::vector<Candidate> cands;
    i = 0;
    while (i < ue_list.size()) {
        switch (ue_list[i].buffer_size > 0 ? 1 : 0) {
            case 1: {
                Candidate c;
                c.idx = static_cast<int>(i);
                /*
                pf_metric
                c.metric = static_cast<double>(ue_list[i].cqi) * static_cast<double>(BYTES_PER_RB)
                           / (ue_list[i].avg_rate + AVG_RATE_BIAS);
                */
                c.metric = pf_metric(ue_list[i]);
                cands.push_back(c);
                break;
            }
            default:
                break;
        }
        i = i + 1;
    }

    std::sort(cands.begin(), cands.end(), metric_greater);

    int remaining_rb = TOTAL_RB;

    // metric 높은 후보부터 RB 1개씩 할당. 한 바퀴 후에도 RB 남으면 반복.
    while (remaining_rb > 0) {
        bool any = false;
        size_t k = 0;
        while (k < cands.size()) {
            UE& ue = ue_list[static_cast<size_t>(cands[k].idx)];
            switch ((ue.buffer_size > 0 && remaining_rb > 0) ? 1 : 0) {
                case 1: {
                    int give = BYTES_PER_RB;
                    switch (give > ue.buffer_size ? 1 : 0) {
                        case 1:
                            give = ue.buffer_size;
                            break;
                        default:
                            break;
                    }
                    ue.buffer_size = ue.buffer_size - give;
                    ue.allocated_this_tti = ue.allocated_this_tti + give;
                    ue.served_bytes = ue.served_bytes + give;
                    remaining_rb = remaining_rb - 1;
                    any = true;
                    break;
                }
                default:
                    break;
            }
            k = k + 1;
        }
        // 더 이상 받을 UE가 없으면(전원 버퍼 고갈) 종료
        switch (any ? 1 : 0) {
            case 0:
                remaining_rb = 0;
                break;
            default:
                break;
        }
    }
}

void schedule_one_tti(std::vector<UE>& ue_list, int tti_ms) {
    allocate_rbs_multi(ue_list);
    update_avg_rates_multi(ue_list);

    std::cout << tti_ms;
    size_t i = 0;
    while (i < ue_list.size()) {
        std::cout << "\tUE" << ue_list[i].id << "=" << ue_list[i].allocated_this_tti
                  << "(cqi" << ue_list[i].cqi << ",avg" << ue_list[i].avg_rate << ")";
        i = i + 1;
    }
    std::cout << std::endl;
}

int main() {
    enable_ansi_stdout();
    std::srand(RNG_SEED);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# multi-UE RB split per TTI, seed=" << RNG_SEED << std::endl;
    std::cout << "TTI\tallocations..." << std::endl;

    std::vector<UE> ue_list;
    ue_list.push_back(make_ue(0, 8000, 12));
    ue_list.push_back(make_ue(1, 8000, 4));
    ue_list.push_back(make_ue(2, 8000, 15));

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_one_tti(ue_list, tti);
        tti = tti + 1;
    }

    std::cout << "-------------------------------------------------" << std::endl;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        std::cout << "UE" << ue.id << " served=" << ue.served_bytes
                  << " remain=" << ue.buffer_size << std::endl;
        i = i + 1;
    }

    return 0;
}
