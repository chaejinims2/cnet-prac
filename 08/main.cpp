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
static const int DRR_QUANTUM = 1500;

struct LoadResult {
    long long out_bytes[3];
    long long drop_bytes[3];
    int q_final[3];
    long long total_out;
    long long total_drop;
    double jain_out;
};

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
    int jitter = (std::rand() % ARRIVAL_JITTER) - half;
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

static int pick_drr(std::vector<UE>& ue_list) {
    size_t i = 0;
    while (i < ue_list.size()) {
        switch (ue_list[i].buffer_size > 0 ? 1 : 0) {
            case 1:
                ue_list[i].deficit_bytes = ue_list[i].deficit_bytes + DRR_QUANTUM;
                break;
            default:
                break;
        }
        i = i + 1;
    }

    int selected = -1;
    int best_def = -1;
    i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch ((ue.buffer_size > 0 && ue.deficit_bytes > best_def) ? 1 : 0) {
            case 1:
                best_def = ue.deficit_bytes;
                selected = static_cast<int>(i);
                break;
            default:
                break;
        }
        i = i + 1;
    }
    return selected;
}

static void serve_ue(std::vector<UE>& ue_list, int selected, int use_drr) {
    switch (selected >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(selected)];
            int allocated = allocate_bytes(ue);
            switch (use_drr) {
                case 1:
                    switch (allocated > ue.deficit_bytes ? 1 : 0) {
                        case 1:
                            allocated = ue.deficit_bytes;
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
            switch (use_drr) {
                case 1:
                    ue.deficit_bytes = ue.deficit_bytes - allocated;
                    break;
                default:
                    break;
            }
            update_avg_rates(ue_list, selected, allocated);
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list, int use_drr) {
    int selected = -1;
    switch (use_drr) {
        case 1:
            selected = pick_drr(ue_list);
            break;
        default:
            selected = pick_pf(ue_list);
            break;
    }
    serve_ue(ue_list, selected, use_drr);
}

static void save_result(const std::vector<UE>& ue_list, LoadResult& out) {
    out.total_out = 0;
    out.total_drop = 0;
    size_t i = 0;
    while (i < ue_list.size()) {
        out.out_bytes[i] = ue_list[i].served_bytes;
        out.drop_bytes[i] = ue_list[i].dropped_bytes;
        out.q_final[i] = ue_list[i].buffer_size;
        out.total_out = out.total_out + ue_list[i].served_bytes;
        out.total_drop = out.total_drop + ue_list[i].dropped_bytes;
        i = i + 1;
    }
    out.jain_out = jain_index(ue_list);
}

static void run_mode(int use_drr, LoadResult& out) {
    std::vector<UE> ue_list = make_ue_list();
    std::srand(RNG_SEED);
    int tti = 1;
    while (tti <= TTI_COUNT) {
        arrive_traffic(ue_list);
        fade_cqi(ue_list);
        schedule_one_tti(ue_list, use_drr);
        tti = tti + 1;
    }
    save_result(ue_list, out);
}

static const char* mode_name(int use_drr) {
    switch (use_drr) {
        case 1:
            return "DRR";
        default:
            return "PF";
    }
}

int main() {
    enable_ansi_stdout();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# DRR vs PF under arrival load, seed=" << RNG_SEED << std::endl;
    std::cout << std::endl;

    LoadResult drr;
    LoadResult pf;
    run_mode(1, drr);
    run_mode(0, pf);

    int mode = 1;
    while (mode >= 0) {
        LoadResult* r = 0;
        switch (mode) {
            case 1:
                r = &drr;
                break;
            default:
                r = &pf;
                break;
        }
        std::cout << "=== " << mode_name(mode) << " ===" << std::endl;
        size_t i = 0;
        while (i < 3) {
            std::cout << "UE" << i << " out=" << r->out_bytes[i]
                      << " drop=" << r->drop_bytes[i]
                      << " q=" << r->q_final[i] << std::endl;
            i = i + 1;
        }
        std::cout << "TOTAL out=" << r->total_out << " drop=" << r->total_drop
                  << " jain=" << r->jain_out << std::endl;
        std::cout << std::endl;
        mode = mode - 1;
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\ttotal_out\ttotal_drop\tjain_out\tue0_out\tue1_out\tue2_out"
              << std::endl;
    std::cout << "DRR\t" << drr.total_out << "\t" << drr.total_drop << "\t" << drr.jain_out
              << "\t" << drr.out_bytes[0] << "\t" << drr.out_bytes[1] << "\t" << drr.out_bytes[2]
              << std::endl;
    std::cout << "PF\t" << pf.total_out << "\t" << pf.total_drop << "\t" << pf.jain_out << "\t"
              << pf.out_bytes[0] << "\t" << pf.out_bytes[1] << "\t" << pf.out_bytes[2]
              << std::endl;
    return 0;
}
