#include <iomanip>
#include <iostream>
#include <vector>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
constexpr const char* kAnsiHighlight = "\033[1;33m";
constexpr const char* kAnsiReset = "\033[0m";

void enable_ansi_stdout() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD mode = 0;
    if (!GetConsoleMode(out, &mode)) {
        return;
    }
    SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

}  // namespace

// 1. 단말(User Equipment) 정의
struct UE {
    int id;
    int buffer_size;      // 코어망에서 내려와 기지국 큐에 쌓인 데이터 양 (Bytes)
    int cqi;              // 채널 품질 (Channel Quality Indicator: 1~15)
                          // 1이면 전파 최악(전송량 낮음), 15면 전파 최상(전송량 높음)
    double weight = 1.0;        // PF 알고리즘용 가중치
    double avg_rate = 0.0;      // 지금까지 누적된 평균 전송 속도 (공평성 계산용)
    /*
        {id, buffer_size, cqi, weight, avg_rate}
        {0, 5000, 12, 1.0, 0.0}, // UE 0: 신호 좋음(12), 데이터 많음
        {1, 5000, 4, 1.0, 0.0},  // UE 1: 신호 나쁨(4), 데이터 많음
        {2, 1000, 15, 1.0, 0.0}  // UE 2: 신호 최상(15), 데이터 적음
    */
};

// 2. 무선 자원 단위 정의 (Resource Block)
const int TOTAL_RB = 100; // 1 TTI(1ms) 동안 나누어 줄 수 있는 총 주파수 자원 양
const int BYTES_PER_RB = 50; // 1 RB(Resource Block)당 전송 가능 데이터 양
// PF: inst·avg_rate를 같은 Bytes/TTI 스케일로 맞추고, 분모에 bias를 두어 공정성 항이 CQI에 묻히지 않게 함
constexpr double AVG_RATE_BIAS = 20.0;
constexpr double IIR_KEEP = 0.85;
constexpr double IIR_NEW = 0.15;

// 스케줄러 함수: 이번 1ms(TTI)에 어떤 단말에 자원(RB)을 줄지 결정
void schedule_proportional_fair(std::vector<UE>& ue_list, int tti_ms) {
    int remaining_rb = TOTAL_RB;

    // 1. 각 단말의 우선순위 가치(Metric) 계산
    // PF 공식: Priority = 현재 보낼 수 있는 가상 속도(CQI 기반) / 과거 평균 속도(avg_rate)
    int selected_index = -1;
    double max_metric = -1.0;

    for (size_t i = 0; i < ue_list.size(); ++i) {
        UE& ue = ue_list[i];
        if (ue.buffer_size <= 0) continue; // 보낼 데이터가 없으면 패스

        // 이번 TTI 최대 할당 가능량과 동일 스케일 (Bytes/TTI)
        double current_achievable_rate = ue.cqi * static_cast<double>(BYTES_PER_RB);

        double denominator = ue.avg_rate + AVG_RATE_BIAS;
        double metric = current_achievable_rate / denominator;

        if (metric > max_metric) {
            max_metric = metric;
            selected_index = static_cast<int>(i);
        }
    }

    int allocated_data = 0;

    // 2. 선택된 단말에 자원 할당 및 상태 업데이트
    if (selected_index != -1) {
        UE& selected_ue = ue_list[static_cast<size_t>(selected_index)];

        // 자원 할당 (RB 예산 내에서 단순화)
        allocated_data = selected_ue.cqi * BYTES_PER_RB;
        const int rb_budget_bytes = remaining_rb * BYTES_PER_RB;
        if (allocated_data > rb_budget_bytes) {
            allocated_data = rb_budget_bytes;
        }
        if (allocated_data > selected_ue.buffer_size) {
            allocated_data = selected_ue.buffer_size;
        }

        selected_ue.buffer_size -= allocated_data;
        
        // 평균 전송 속도 업데이트 (IIR 필터 방식)
        selected_ue.avg_rate = IIR_KEEP * selected_ue.avg_rate + IIR_NEW * allocated_data;
    }

    // 선택받지 못한 단말들도 평균 속도 업데이트 (속도가 떨어지므로 다음 턴에 우선순위 올라감)
    for (size_t i = 0; i < ue_list.size(); ++i) {
        if (static_cast<int>(i) != selected_index) {
            UE& ue = ue_list[i];
            ue.avg_rate = IIR_KEEP * ue.avg_rate;
        }
    }

    if (selected_index != -1) {
        const UE& selected_ue = ue_list[static_cast<size_t>(selected_index)];
        std::cout << tti_ms << "\t" << selected_ue.id << "\t" << allocated_data;
        for (size_t i = 0; i < ue_list.size(); ++i) {
            const double r = ue_list[i].avg_rate;
            const bool highlight = static_cast<int>(i) == selected_index;
            std::cout << "\t";
            if (highlight) {
                // std::cout << kAnsiHighlight;
            }
            std::cout << r << "(" << ue_list[i].cqi << ")";
            if (highlight) {
                // std::cout << kAnsiReset;
            }
        }
        std::cout << std::endl;
    }
}


int main() {
    enable_ansi_stdout();

    // 3개의 가상 단말 생성
    std::vector<UE> ue_list = {
        {0, 5000, 12}, // UE 0: 신호 좋음(12), 데이터 많음
        {1, 5000, 4},  // UE 1: 신호 나쁨(4), 데이터 많음
        {2, 1000, 15}  // UE 2: 신호 최상(15), 데이터 적음
    };
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "TTI(ms)" << "\t" << "UE(id)" << "\t" << "Data(Bytes)"
              << "\t" << "avg_R0(cqi)" << "\t" << "avg_R1(cqi)" << "\t" << "avg_R2(cqi)"
              << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;

    // 100ms 동안 무선 통신이 흘러간다고 가정 (100 TTI Loop)
    for (int tti = 1; tti <= 100; tti++) {
        // [실제 환경 모사] 매 ms마다 단말의 무선 신호(CQI)가 계속 흔들림 (Fading 현상)
        for (auto& ue : ue_list) {
            ue.cqi = (rand() % 15) + 1; // 1 ~ 15 사이 랜덤 생성
        }

        schedule_proportional_fair(ue_list, tti);
    }

    return 0;
}
