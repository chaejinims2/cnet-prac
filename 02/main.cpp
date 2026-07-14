#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"

// (Linux sched.h 의 SCHED_RR 등과 이름 충돌을 피하기 위해 ALGO_ 접두사 사용)
enum AlgoKind {
    ALGO_RR = 0,
    ALGO_MAX_CQI = 1,
    ALGO_PF = 2
};

static const int TTI_COUNT = 100;
static const int ALGO_COUNT = 3;

static std::vector<UE> g_ue_list;
static int g_rr_cursor = 0;
static int g_algo = ALGO_PF;

static const char* algo_name(int algo) {
    switch (algo) {
        case ALGO_RR:
            return "RR";
        case ALGO_MAX_CQI:
            return "Max-CQI";
        case ALGO_PF:
            return "PF";
        default:
            return "UNKNOWN";
    }
}

void init() {
    // UE 초기값만 준비. 알고리즘마다 reset_scenario() 에서 다시 깐다.
}

void input() {
    // 고정 시나리오. 필요하면 buffer_size / TTI_COUNT 를 여기서 바꿔본다.
}

static void reset_scenario() {
    g_ue_list.clear();
    g_rr_cursor = 0;
    // 100 TTI 안에 고갈되지 않게 크게 둠 → 알고리즘별 분배 차이가 Jain에 반영됨
    g_ue_list.push_back(make_ue(0, 100000, 12));
    g_ue_list.push_back(make_ue(1, 100000, 4));
    g_ue_list.push_back(make_ue(2, 100000, 15));
    // 동일 seed → 알고리즘마다 같은 CQI 시퀀스
    std::srand(RNG_SEED);
}

static int pick_rr(const std::vector<UE>& ue_list) {
    const int n = static_cast<int>(ue_list.size());
    int i = 0;
    while (i < n) {
        const int idx = (g_rr_cursor + i) % n;
        switch (ue_list[static_cast<size_t>(idx)].buffer_size > 0 ? 1 : 0) {
            case 1:
                g_rr_cursor = (idx + 1) % n;
                return idx;
            default:
                break;
        }
        i = i + 1;
    }
    return -1;
}

static int pick_max_cqi(const std::vector<UE>& ue_list) {
    int selected = -1;
    int best_cqi = -1;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1:
                switch (ue.cqi > best_cqi ? 1 : 0) {
                    case 1:
                        best_cqi = ue.cqi;
                        selected = static_cast<int>(i);
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        i = i + 1;
    }
    return selected;
}

static int select_ue(const std::vector<UE>& ue_list) {
    switch (g_algo) {
        case ALGO_RR:
            return pick_rr(ue_list);
        case ALGO_MAX_CQI:
            return pick_max_cqi(ue_list);
        case ALGO_PF:
            return pick_pf(ue_list);
        default:
            return -1;
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list, int tti_ms) {
    (void)tti_ms;
    const int selected = select_ue(ue_list);
    switch (selected >= 0 ? 1 : 0) {
        case 0:
            return;
        default:
            break;
    }

    UE& ue = ue_list[static_cast<size_t>(selected)];
    const int allocated = allocate_bytes(ue);
    ue.buffer_size = ue.buffer_size - allocated;
    ue.served_bytes = ue.served_bytes + allocated;
    update_avg_rates(ue_list, selected, allocated);
}

static void print_summary(int algo) {
    long long total = 0;
    size_t i = 0;
    std::cout << "=== " << algo_name(algo) << " ===" << std::endl;
    std::cout << "UE\tserved_bytes\tremain_buffer" << std::endl;
    while (i < g_ue_list.size()) {
        const UE& ue = g_ue_list[i];
        total = total + ue.served_bytes;
        std::cout << ue.id << "\t" << ue.served_bytes << "\t" << ue.buffer_size << std::endl;
        i = i + 1;
    }
    std::cout << "total_bytes\t" << total << std::endl;
    std::cout << "jain_index\t" << jain_index(g_ue_list) << std::endl;
    std::cout << std::endl;
}

void run() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# compare RR / Max-CQI / PF, seed=" << RNG_SEED
              << ", tti=" << TTI_COUNT << std::endl;
    std::cout << std::endl;

    int algo = 0;
    while (algo < ALGO_COUNT) {
        g_algo = algo;
        reset_scenario();

        int tti = 1;
        while (tti <= TTI_COUNT) {
            fade_cqi(g_ue_list);
            schedule_one_tti(g_ue_list, tti);
            tti = tti + 1;
        }

        print_summary(algo);
        algo = algo + 1;
    }
}

void solution() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "compare tip: Max-CQI thruput↑ fairness↓ / RR opposite / PF middle"
              << std::endl;
}

int main() {
    init();
    input();
    run();
    solution();
    return 0;
}
