#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

static const int TTI_COUNT = 100;
static const int GOOD_RB = 60;
static const int BAD_RB = 40;

struct RunResult {
    long long served[3];
    int good_used[3];
    int bad_used[3];
};

static std::vector<UE> make_ue_list() {
    std::vector<UE> ue_list;
    ue_list.push_back(make_ue(0, 10000, 12));
    ue_list.push_back(make_ue(1, 10000, 8));
    ue_list.push_back(make_ue(2, 10000, 15));
    return ue_list;
}

static int pick_best_pf(const std::vector<UE>& ue_list) {
    return pick_pf(ue_list);
}

static int pick_for_band(const std::vector<UE>& ue_list, int band_good) {
    int selected = -1;
    double best = -1.0;
    size_t i = 0;
    while (i < ue_list.size()) {
        const UE& ue = ue_list[i];
        switch (ue.buffer_size > 0 ? 1 : 0) {
            case 1: {
                double inst = pf_inst_rate(ue);
                switch (band_good) {
                    case 0:
                        inst = inst * 0.5;
                        break;
                    default:
                        break;
                }
                const double metric = inst / (ue.avg_rate + AVG_RATE_BIAS);
                switch (metric > best ? 1 : 0) {
                    case 1:
                        best = metric;
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

static int bytes_for_band(int cqi, int rb_count, int band_good) {
    int eff_cqi = cqi;
    switch (band_good) {
        case 0:
            eff_cqi = cqi / 2;
            switch (eff_cqi < 1 ? 1 : 0) {
                case 1:
                    eff_cqi = 1;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    int allocated = eff_cqi * BYTES_PER_RB;
    const int cap = rb_count * BYTES_PER_RB;
    switch (allocated > cap ? 1 : 0) {
        case 1:
            allocated = cap;
            break;
        default:
            break;
    }
    return allocated;
}

static void give_bytes(UE& ue, int bytes, int band_good, int good_cnt[3], int bad_cnt[3]) {
    switch (bytes > ue.buffer_size ? 1 : 0) {
        case 1:
            bytes = ue.buffer_size;
            break;
        default:
            break;
    }
    ue.buffer_size = ue.buffer_size - bytes;
    ue.served_bytes = ue.served_bytes + bytes;
    switch (band_good) {
        case 1:
            good_cnt[ue.id] = good_cnt[ue.id] + 1;
            break;
        default:
            bad_cnt[ue.id] = bad_cnt[ue.id] + 1;
            break;
    }
}

static void schedule_freq_aware(std::vector<UE>& ue_list, int good_cnt[3], int bad_cnt[3]) {
    int good_sel = pick_for_band(ue_list, 1);
    switch (good_sel >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(good_sel)];
            const int bytes = bytes_for_band(ue.cqi, GOOD_RB, 1);
            give_bytes(ue, bytes, 1, good_cnt, bad_cnt);
            update_avg_rates(ue_list, good_sel, bytes);
            break;
        }
        default:
            update_avg_rates(ue_list, -1, 0);
            break;
    }

    int bad_sel = pick_for_band(ue_list, 0);
    switch (bad_sel >= 0 ? 1 : 0) {
        case 1: {
            UE& ue = ue_list[static_cast<size_t>(bad_sel)];
            const int bytes = bytes_for_band(ue.cqi, BAD_RB, 0);
            give_bytes(ue, bytes, 0, good_cnt, bad_cnt);
            update_avg_rates(ue_list, bad_sel, bytes);
            break;
        }
        default:
            break;
    }
}

static void schedule_flat_pf(std::vector<UE>& ue_list) {
    const int selected = pick_best_pf(ue_list);
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

static void run_freq_aware(RunResult& out) {
    std::vector<UE> ue_list = make_ue_list();
    int good_cnt[3];
    int bad_cnt[3];
    good_cnt[0] = 0;
    good_cnt[1] = 0;
    good_cnt[2] = 0;
    bad_cnt[0] = 0;
    bad_cnt[1] = 0;
    bad_cnt[2] = 0;
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_freq_aware(ue_list, good_cnt, bad_cnt);
        tti = tti + 1;
    }

    size_t i = 0;
    while (i < ue_list.size()) {
        out.served[i] = ue_list[i].served_bytes;
        out.good_used[i] = good_cnt[i];
        out.bad_used[i] = bad_cnt[i];
        i = i + 1;
    }
}

static void run_flat_pf(RunResult& out) {
    std::vector<UE> ue_list = make_ue_list();
    std::srand(RNG_SEED);

    int tti = 1;
    while (tti <= TTI_COUNT) {
        fade_cqi(ue_list);
        schedule_flat_pf(ue_list);
        tti = tti + 1;
    }

    size_t i = 0;
    while (i < ue_list.size()) {
        out.served[i] = ue_list[i].served_bytes;
        out.good_used[i] = 0;
        out.bad_used[i] = 0;
        i = i + 1;
    }
}

int main() {
    enable_ansi_stdout();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# freq good/bad RB matching vs flat PF, seed=" << RNG_SEED << std::endl;
    std::cout << "GOOD_RB=" << GOOD_RB << " BAD_RB=" << BAD_RB << std::endl;
    std::cout << std::endl;

    RunResult freq;
    RunResult flat;
    run_freq_aware(freq);
    run_flat_pf(flat);

    std::cout << "=== freq_match ===" << std::endl;
    size_t i = 0;
    while (i < 3) {
        std::cout << "UE" << i << " served=" << freq.served[i] << std::endl;
        i = i + 1;
    }
    std::cout << std::endl;

    std::cout << "=== flat_PF ===" << std::endl;
    i = 0;
    while (i < 3) {
        std::cout << "UE" << i << " served=" << flat.served[i] << std::endl;
        i = i + 1;
    }
    std::cout << std::endl;

    long long total_freq = freq.served[0] + freq.served[1] + freq.served[2];
    long long total_flat = flat.served[0] + flat.served[1] + flat.served[2];

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "mode\tue0_srv\tue1_srv\tue2_srv\ttotal\tjain" << std::endl;
    std::vector<UE> tmp;
    i = 0;
    while (i < 3) {
        UE u = make_ue(static_cast<int>(i), 0, 10);
        u.served_bytes = freq.served[i];
        tmp.push_back(u);
        i = i + 1;
    }
    std::cout << "freq_match\t" << freq.served[0] << "\t" << freq.served[1] << "\t"
              << freq.served[2] << "\t" << total_freq << "\t" << jain_index(tmp) << std::endl;

    tmp.clear();
    i = 0;
    while (i < 3) {
        UE u = make_ue(static_cast<int>(i), 0, 10);
        u.served_bytes = flat.served[i];
        tmp.push_back(u);
        i = i + 1;
    }
    std::cout << "flat_PF\t" << flat.served[0] << "\t" << flat.served[1] << "\t"
              << flat.served[2] << "\t" << total_flat << "\t" << jain_index(tmp) << std::endl;
    return 0;
}
