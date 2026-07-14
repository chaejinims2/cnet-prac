#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

static const int TTI_COUNT = 200;

static const int ARRIVAL_MEAN_UE0 = 400;
static const int ARRIVAL_MEAN_UE1 = 400;
static const int ARRIVAL_MEAN_UE2 = 200;
static const int ARRIVAL_JITTER = 100;
static const int BUFFER_CAP = 20000;

struct LoadResult {
    long long in_bytes[3];
    long long out_bytes[3];
    long long drop_bytes[3];
    int q_final[3];
    long long total_in;
    long long total_out;
    long long total_drop;
    double jain_out;
};

int select_algo_custom(const std::vector<UE>& ue_list, int algo, int& rr_cursor) {
    switch (algo) {
        case ALGO_RR:
            return pick_rr(ue_list, rr_cursor);
        default:
            return select_algo(ue_list, algo);
    }
}

static std::vector<UE> make_ue_list() {
    std::vector<UE> ue_list;
    UE u0 = make_ue(0, 0, 10);
    u0.arrival_mean = ARRIVAL_MEAN_UE0;
    UE u1 = make_ue(1, 0, 10);
    u1.arrival_mean = ARRIVAL_MEAN_UE1;
    UE u2 = make_ue(2, 0, 10);
    u2.arrival_mean = ARRIVAL_MEAN_UE2;
    ue_list.push_back(u0);
    ue_list.push_back(u1);
    ue_list.push_back(u2);
    return ue_list;
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

static void schedule_one_tti(std::vector<UE>& ue_list, int algo, int& rr_cursor) {
    const int selected = select_algo_custom(ue_list, algo, rr_cursor);
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            const int allocated = allocate_bytes(ue);
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

static void fill_result(const std::vector<UE>& ue_list, LoadResult& out) {
    out.total_in = 0;
    out.total_out = 0;
    out.total_drop = 0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        out.in_bytes[i] = ue.arrived_bytes;
        out.out_bytes[i] = ue.served_bytes;
        out.drop_bytes[i] = ue.dropped_bytes;
        out.q_final[i] = ue.buffer_size;
        out.total_in = out.total_in + ue.arrived_bytes;
        out.total_out = out.total_out + ue.served_bytes;
        out.total_drop = out.total_drop + ue.dropped_bytes;
        i = i + 1;
    }
    out.jain_out = jain_index(ue_list);
}

static void print_mode_summary(int algo, const LoadResult& r) {
    std::cout << "=== " << algo_name(algo) << " ===" << std::endl;
    size_t i = 0;
    while (i < 3) {
        std::cout << "UE" << i
                  << " in=" << r.in_bytes[i]
                  << " out=" << r.out_bytes[i]
                  << " drop=" << r.drop_bytes[i]
                  << " q=" << r.q_final[i] << std::endl;
        i = i + 1;
    }
    std::cout << "TOTAL in=" << r.total_in
              << " out=" << r.total_out
              << " drop=" << r.total_drop
              << " jain_out=" << r.jain_out << std::endl;
    std::cout << std::endl;
}

static void run_one_algo(int algo, LoadResult& out) {
    std::vector<UE> ue_list = make_ue_list();
    int rr_cursor = 0;
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        arrive_traffic(ue_list);
        fade_cqi(ue_list);
        schedule_one_tti(ue_list, algo, rr_cursor);
        tti = tti + 1;
    }

    fill_result(ue_list, out);
    print_mode_summary(algo, out);
}

int main() {
    enable_ansi_stdout();

    LoadResult results[ALGO_KIND_COUNT];

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# traffic arrival load compare RR/Max-CQI/PF, seed=" << RNG_SEED
              << ", tti=" << TTI_COUNT << std::endl;
    std::cout << std::endl;

    int algo = 0;
    while (algo < ALGO_KIND_COUNT) {
        run_one_algo(algo, results[algo]);
        algo = algo + 1;
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "algo\ttotal_in\ttotal_out\ttotal_drop\tjain_out"
              << "\tue0_drop\tue1_drop\tue2_drop" << std::endl;

    algo = 0;
    while (algo < ALGO_KIND_COUNT) {
        const LoadResult& r = results[algo];
        std::cout << algo_name(algo)
                  << "\t" << r.total_in
                  << "\t" << r.total_out
                  << "\t" << r.total_drop
                  << "\t" << r.jain_out
                  << "\t" << r.drop_bytes[0]
                  << "\t" << r.drop_bytes[1]
                  << "\t" << r.drop_bytes[2]
                  << std::endl;
        algo = algo + 1;
    }
    std::cout << "compare tip: load↑ 시 RR은 out↓·drop↑, Max-CQI는 thruput↑, PF는 중간"
              << std::endl;
    return 0;
}
