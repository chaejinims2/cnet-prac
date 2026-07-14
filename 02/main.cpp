#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

// [실습] 여기를 바꿔가며 비교: 0=RR, 1=Max-CQI, 2=PF
// (Linux sched.h 의 SCHED_RR 등과 이름 충돌을 피하기 위해 ALGO_ 접두사 사용)
enum AlgoKind {
    ALGO_RR = 0,
    ALGO_MAX_CQI = 1,
    ALGO_PF = 2
};

static const int ALGO = ALGO_PF;
static const unsigned int RNG_SEED = 42;
static const int TOTAL_RB = 100;
static const int BYTES_PER_RB = 50;
static const int TTI_COUNT = 100;
static const double AVG_RATE_BIAS = 20.0;
static const double IIR_KEEP = 0.85;
static const double IIR_NEW = 0.15;

struct UE {
    int id;
    int buffer_size;
    int cqi;
    double avg_rate;
    long long served_bytes;
};

static std::vector<UE> g_ue_list;
static int g_rr_cursor = 0;

void init() {
    g_ue_list.clear();
    g_rr_cursor = 0;

    UE u0;
    u0.id = 0;
    u0.buffer_size = 5000;
    u0.cqi = 12;
    u0.avg_rate = 0.0;
    u0.served_bytes = 0;

    UE u1;
    u1.id = 1;
    u1.buffer_size = 5000;
    u1.cqi = 4;
    u1.avg_rate = 0.0;
    u1.served_bytes = 0;

    UE u2;
    u2.id = 2;
    u2.buffer_size = 1000;
    u2.cqi = 15;
    u2.avg_rate = 0.0;
    u2.served_bytes = 0;

    g_ue_list.push_back(u0);
    g_ue_list.push_back(u1);
    g_ue_list.push_back(u2);

    std::srand(RNG_SEED);
}

void input() {
    // 고정 시나리오. 필요하면 buffer_size / TTI_COUNT 를 여기서 바꿔본다.
}

static int pick_rr(const std::vector<UE>& ue_list) {
    const int n = static_cast<int>(ue_list.size());
    int i = 0;
    while (i < n) {
        const int idx = (g_rr_cursor + i) % n;
        if (ue_list[static_cast<size_t>(idx)].buffer_size > 0) {
            g_rr_cursor = (idx + 1) % n;
            return idx;
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

static int pick_pf(const std::vector<UE>& ue_list) {
    int selected = -1;
    double max_metric = -1.0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1: {
                const double inst =
                    static_cast<double>(ue.cqi) * static_cast<double>(BYTES_PER_RB);
                const double metric = inst / (ue.avg_rate + AVG_RATE_BIAS);
                switch (metric > max_metric ? 1 : 0) {
                    case 1:
                        max_metric = metric;
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
    return selected;
}

static int select_ue(const std::vector<UE>& ue_list) {
    switch (ALGO) {
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

static int allocate_bytes(UE& ue) {
    int allocated = ue.cqi * BYTES_PER_RB;
    const int rb_budget_bytes = TOTAL_RB * BYTES_PER_RB;
    switch (allocated > rb_budget_bytes ? 1 : 0) {
        case 1:
            allocated = rb_budget_bytes;
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
    return allocated;
}

static void update_avg_rates(std::vector<UE>& ue_list, int selected, int allocated) {
    size_t i = 0;
    while (i < ue_list.size()) {
        UE& ue = ue_list[i];
        switch (static_cast<int>(i) == selected ? 1 : 0) {
            case 1:
                ue.avg_rate = IIR_KEEP * ue.avg_rate + IIR_NEW * static_cast<double>(allocated);
                break;
            default:
                ue.avg_rate = IIR_KEEP * ue.avg_rate;
                break;
        }
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

    std::cout << tti_ms << "\t" << ue.id << "\t" << allocated;
    size_t i = 0;
    while (i < ue_list.size()) {
        std::cout << "\t" << ue_list[i].avg_rate << "(" << ue_list[i].cqi << ")";
        i = i + 1;
    }
    std::cout << std::endl;
}

static double jain_index(const std::vector<UE>& ue_list) {
    double sum = 0.0;
    double sum_sq = 0.0;
    const double n = static_cast<double>(ue_list.size());
    size_t i = 0;
    while (i < ue_list.size()) {
        const double r = static_cast<double>(ue_list[i].served_bytes);
        sum = sum + r;
        sum_sq = sum_sq + r * r;
        i = i + 1;
    }
    switch (sum_sq <= 0.0 ? 1 : 0) {
        case 1:
            return 0.0;
        default:
            return (sum * sum) / (n * sum_sq);
    }
}

void run() {
    const char* name = "PF";
    switch (ALGO) {
        case ALGO_RR:
            name = "RR";
            break;
        case ALGO_MAX_CQI:
            name = "Max-CQI";
            break;
        case ALGO_PF:
            name = "PF";
            break;
        default:
            name = "UNKNOWN";
            break;
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# scheduler=" << name << " seed=" << RNG_SEED << std::endl;
    std::cout << "TTI(ms)\tUE(id)\tData(Bytes)"
              << "\tavg_R0(cqi)\tavg_R1(cqi)\tavg_R2(cqi)" << std::endl;

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(g_ue_list);
        schedule_one_tti(g_ue_list, tti);
        tti = tti + 1;
    }
}

void solution() {
    long long total = 0;
    size_t i = 0;
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "UE\tserved_bytes\tremain_buffer" << std::endl;
    while (i < g_ue_list.size()) {
        const UE& ue = g_ue_list[i];
        total = total + ue.served_bytes;
        std::cout << ue.id << "\t" << ue.served_bytes << "\t" << ue.buffer_size << std::endl;
        i = i + 1;
    }
    std::cout << "total_bytes\t" << total << std::endl;
    std::cout << "jain_index\t" << jain_index(g_ue_list) << std::endl;
}

int main() {
    init();
    input();
    run();
    solution();
    return 0;
}
