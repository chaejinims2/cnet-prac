#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

static const unsigned int RNG_SEED = 42;
static const int TOTAL_RB = 100;
static const int BYTES_PER_RB = 50;
static const int TTI_COUNT = 50;
static const double AVG_RATE_BIAS = 20.0;
static const double IIR_KEEP = 0.85;
static const double IIR_NEW = 0.15;

struct UE {
    int id;
    int buffer_size;
    int cqi;
    double avg_rate;
    long long served_bytes;
    int allocated_this_tti;
};

struct Candidate {
    int idx;
    double metric;
};

static std::vector<UE> g_ue_list;

void init() {
    g_ue_list.clear();

    UE u0 = {0, 8000, 12, 0.0, 0, 0};
    UE u1 = {1, 8000, 4, 0.0, 0, 0};
    UE u2 = {2, 8000, 15, 0.0, 0, 0};
    g_ue_list.push_back(u0);
    g_ue_list.push_back(u1);
    g_ue_list.push_back(u2);

    std::srand(RNG_SEED);
}

void input() {
}

static bool metric_greater(const Candidate& a, const Candidate& b) {
    return a.metric > b.metric;
}

static double pf_metric(const UE& ue) {
    const double inst = static_cast<double>(ue.cqi) * static_cast<double>(BYTES_PER_RB);
    return inst / (ue.avg_rate + AVG_RATE_BIAS);
}

// [실습] 핵심: remaining_rb 를 여러 UE에 나눠 주기
static void allocate_rbs_multi(std::vector<UE>& ue_list) {
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

    /*
     * TODO(실습):
     * - metric 높은 후보부터 순회
     * - 아직 buffer가 있고 remaining_rb > 0 이면 RB 1개 할당
     *   bytes = BYTES_PER_RB  (간단 버전; CQI 반영은 선택)
     * - buffer에서 차감, allocated_this_tti / served_bytes 증가
     * - remaining_rb 감소
     * - 한 바퀴 돌았는데도 RB가 남으면 다시 앞에서부터 반복(라운드)
     *
     * 아래는 “동작하는 뼈대” 예시 구현이다. 직접 고쳐도 된다.
     */
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
        switch (any ? 1 : 0) {
            case 0:
                remaining_rb = 0;
                break;
            default:
                break;
        }
    }
}

static void update_avg_rates(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        UE& ue = ue_list[i];
        ue.avg_rate =
            IIR_KEEP * ue.avg_rate + IIR_NEW * static_cast<double>(ue.allocated_this_tti);
        i = i + 1;
    }
}

static void fade_cqi(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        ue_list[i].cqi = (std::rand() % 15) + 1;
        i = i + 1;
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list, int tti_ms) {
    allocate_rbs_multi(ue_list);
    update_avg_rates(ue_list);

    std::cout << tti_ms;
    size_t i = 0;
    while (i < ue_list.size()) {
        std::cout << "\tUE" << ue_list[i].id << "=" << ue_list[i].allocated_this_tti
                  << "(cqi" << ue_list[i].cqi << ",avg" << ue_list[i].avg_rate << ")";
        i = i + 1;
    }
    std::cout << std::endl;
}

void run() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# multi-UE RB split per TTI, seed=" << RNG_SEED << std::endl;
    std::cout << "TTI\tallocations..." << std::endl;

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(g_ue_list);
        schedule_one_tti(g_ue_list, tti);
        tti = tti + 1;
    }
}

void solution() {
    std::cout << "-------------------------------------------------" << std::endl;
    size_t i = 0;
    while (i < g_ue_list.size()) {
        const UE& ue = g_ue_list[i];
        std::cout << "UE" << ue.id << " served=" << ue.served_bytes
                  << " remain=" << ue.buffer_size << std::endl;
        i = i + 1;
    }
}

int main() {
    init();
    input();
    run();
    solution();
    return 0;
}
