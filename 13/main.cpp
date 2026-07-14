#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/cnet-prac.h"
#include "../include/debug.h"

struct TraceRow {
    int tti;
    int cqi[3];
    int arr[3];
};

static bool load_trace(const char* path, std::vector<TraceRow>& rows) {
    std::ifstream in(path);
    switch (in.good() ? 1 : 0) {
        case 0:
            return false;
        default:
            break;
    }
    std::string line;
    switch (std::getline(in, line) ? 1 : 0) {
        case 0:
            return false;
        default:
            break;
    }
    while (std::getline(in, line)) {
        switch (line.empty() ? 1 : 0) {
            case 1:
                continue;
            default:
                break;
        }
        TraceRow row;
        char comma = 0;
        std::stringstream ss(line);
        ss >> row.tti >> comma >> row.cqi[0] >> comma >> row.cqi[1] >> comma >> row.cqi[2]
            >> comma >> row.arr[0] >> comma >> row.arr[1] >> comma >> row.arr[2];
        rows.push_back(row);
    }
    return rows.size() > 0;
}

static std::vector<UE> make_ue_list() {
    std::vector<UE> ue_list;
    ue_list.push_back(make_ue(0, 0, 10));
    ue_list.push_back(make_ue(1, 0, 10));
    ue_list.push_back(make_ue(2, 0, 10));
    return ue_list;
}

static void apply_trace(std::vector<UE>& ue_list, const TraceRow& row) {
    size_t i = 0;
    while (i < ue_list.size()) {
        ue_list[i].cqi = row.cqi[i];
        ue_list[i].buffer_size = ue_list[i].buffer_size + row.arr[i];
        ue_list[i].arrived_bytes = ue_list[i].arrived_bytes + row.arr[i];
        i = i + 1;
    }
}

static void schedule_one_tti(std::vector<UE>& ue_list) {
    const int selected = pick_pf(ue_list);
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

int main() {
    enable_ansi_stdout();
    std::vector<TraceRow> rows;
    switch (load_trace("trace.csv", rows) ? 1 : 0) {
        case 0:
            std::cout << "trace load failed" << std::endl;
            return 1;
        default:
            break;
    }

    std::vector<UE> ue_list = make_ue_list();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "# trace player (no rand), rows=" << rows.size() << std::endl;
    std::cout << "TTI\tUE\tData\tq0\tq1\tq2" << std::endl;

    size_t r = 0;
    while (r < rows.size()) {
        apply_trace(ue_list, rows[r]);
        schedule_one_tti(ue_list);
        switch ((rows[r].tti % 10) == 0 ? 1 : 0) {
            case 1: {
                std::cout << rows[r].tti << "\tPF\t-";
                size_t i = 0;
                while (i < ue_list.size()) {
                    std::cout << "\t" << ue_list[i].buffer_size;
                    i = i + 1;
                }
                std::cout << std::endl;
                break;
            }
            default:
                break;
        }
        r = r + 1;
    }

    std::cout << "-------------------------------------------------" << std::endl;
    long long total_in = 0;
    long long total_out = 0;
    size_t i = 0;
    while (i < ue_list.size()) {
        total_in = total_in + ue_list[i].arrived_bytes;
        total_out = total_out + ue_list[i].served_bytes;
        std::cout << "UE" << i << " in=" << ue_list[i].arrived_bytes
                  << " out=" << ue_list[i].served_bytes
                  << " q=" << ue_list[i].buffer_size << std::endl;
        i = i + 1;
    }
    std::cout << "TOTAL in=" << total_in << " out=" << total_out
              << " jain=" << jain_index(ue_list) << std::endl;
    return 0;
}
