#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "deque/segment_deque.h"
#include "sorting_station/sorting_station.h"
#include "hanoi/hanoi.h"
#include "gui/deque_visualizer.h"
#include "gui/benchmarks.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

MutableSegmentedDeque<int> g_deque;
int g_input_value = 0;
static int g_benchmark_sizes[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000};
static const int g_benchmark_sizes_count = sizeof(g_benchmark_sizes) / sizeof(g_benchmark_sizes[0]);
static double g_benchmark_plot_sizes[g_benchmark_sizes_count] = {0};
static BenchmarkParams g_benchmark_params = {g_benchmark_sizes, g_benchmark_sizes_count, 10000, 10 * 1024 * 1024, 50};

static double g_deque_push_front_times[g_benchmark_sizes_count] = {0};
static double g_array_push_front_times[g_benchmark_sizes_count] = {0};
static double g_list_push_front_times[g_benchmark_sizes_count] = {0};

static double g_deque_get_times[g_benchmark_sizes_count] = {0};
static double g_array_get_times[g_benchmark_sizes_count] = {0};
static double g_list_get_times[g_benchmark_sizes_count] = {0};

static double g_mem_deque[g_benchmark_sizes_count] = {0};
static double g_mem_array[g_benchmark_sizes_count] = {0};
static double g_mem_list[g_benchmark_sizes_count] = {0};

static double g_os_deque[g_benchmark_sizes_count] = {0};
static double g_os_array[g_benchmark_sizes_count] = {0};
static double g_os_list[g_benchmark_sizes_count] = {0};

static bool g_benchmarks_done = false;

int g_map_choice = 0;
const char* g_map_funcs[] = {"x * 2", "x squared", "-x"};
int map_double(const int& x) { return x * 2; }
int map_square(const int& x) { return x * x; }
int map_negative(const int& x) { return -x; }

int g_where_choice = 0;
const char* g_where_preds[] = {"x > 0", "x is even", "x < 10"};
bool where_positive(const int& x) { return x > 0; }
bool where_even(const int& x) { return x % 2 == 0; }
bool where_less_10(const int& x) { return x < 10; }

int g_sort_order = 0;
const char* g_sort_orders[] = {"Ascending", "Descending"};
bool sort_less(const int& a, const int& b) { return a < b; }
bool sort_greater(const int& a, const int& b) { return a > b; }

int (*g_map_handlers[])(const int&) = {map_double, map_square, map_negative};
bool (*g_where_handlers[])(const int&) = {where_positive, where_even, where_less_10};
bool (*g_sort_handlers[])(const int&, const int&) = {sort_less, sort_greater};

MutableSegmentedDeque<Wagon> g_station_train;            // Исходный поезд
MutableSegmentedDeque<StationStep> g_station_steps;      // Последовательность шагов
int g_station_current_step = -1;                          // Текущий шаг -1 - до начала

ImU32 wagon_colors[] = {
    IM_COL32(231, 76, 60, 255),   // тип 1 — красный
    IM_COL32(52, 152, 219, 255),  // тип 2 — синий
    IM_COL32(46, 204, 113, 255),  // тип 3 — зелёный
    IM_COL32(241, 196, 15, 255),  // тип 4 — жёлтый
    IM_COL32(155, 89, 182, 255),  // тип 5 — фиолетовый
};
ImU32 get_wagon_color(int type) {
    return wagon_colors[(type - 1) % 5];
}

MutableSegmentedDeque<Wagon> g_station_train_current;
MutableSegmentedDeque<Wagon> g_station_result_current;
MutableSegmentedDeque<Wagon>* g_station_sidings_current = nullptr;
int g_station_siding_count = 0;
MutableSegmentedDeque<int> g_station_types;

bool g_station_auto_play = false;
double g_station_last_step_time = 0;
float g_station_speed = 0.5f;

void station_apply_step(int step_idx) {
    StationStep s = g_station_steps.get(step_idx);
    Wagon w;
    
    switch (s.action) {
        case StationAction::PopFromTrain:
            g_station_train_current.pop_front(&w);
            break;
        case StationAction::PushToSiding:
            g_station_sidings_current[s.siding_index].push_back(s.wagon);
            break;
        case StationAction::PushToResult:
            g_station_result_current.push_back(s.wagon);
            break;
        case StationAction::MoveSidingToResult:
            g_station_sidings_current[s.siding_index].pop_front(&w);
            g_station_result_current.push_back(s.wagon);
            break;
    }
}

void station_undo_step(int step_idx) {
    StationStep s = g_station_steps.get(step_idx);
    Wagon w;
    
    switch (s.action) {
        case StationAction::PopFromTrain:
            g_station_train_current.push_front(s.wagon);
            break;
        case StationAction::PushToSiding:
            g_station_sidings_current[s.siding_index].pop_back(&w);
            break;
        case StationAction::PushToResult:
            g_station_result_current.pop_back(&w);
            break;
        case StationAction::MoveSidingToResult:
            g_station_result_current.pop_back(&w);
            g_station_sidings_current[s.siding_index].push_front(s.wagon);
            break;
    }
}

void station_reset_state() {
    g_station_train_current = g_station_train;
    g_station_result_current = MutableSegmentedDeque<Wagon>();
    
    if (g_station_sidings_current) delete[] g_station_sidings_current;

    g_station_types = collect_types(g_station_train);
    g_station_siding_count = g_station_types.get_count();
    g_station_sidings_current = new MutableSegmentedDeque<Wagon>[g_station_siding_count];
    
    g_station_current_step = -1;
}

void station_next_step() {
    if (g_station_current_step >= g_station_steps.get_count() - 1) return;

    g_station_current_step++;
    station_apply_step(g_station_current_step);
}

void station_prev_step() {
    if (g_station_current_step < 0) return;

    station_undo_step(g_station_current_step);
    g_station_current_step--;
}

void draw_wagon(ImDrawList* draw, ImVec2 pos, const Wagon& w, float width, float height) {
    ImVec2 p1 = pos;
    ImVec2 p2(pos.x + width, pos.y + height);
    
    draw->AddRectFilled(p1, p2, get_wagon_color(w.first()), 4.0f);
    draw->AddRect(p1, p2, IM_COL32(255, 255, 255, 200), 4.0f, 0, 1.5f);

    char text[16];
    snprintf(text, sizeof(text), "t%d,%d", w.first(), w.second());
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 text_pos(pos.x + (width - text_size.x) / 2, 
                    pos.y + (height - text_size.y) / 2);
    draw->AddText(text_pos, IM_COL32(255, 255, 255, 255), text);
}

MutableSegmentedDeque<Ring> g_hanoi_initial_rings;        // Исходные кольца
MutableSegmentedDeque<HanoiMove> g_hanoi_moves;           // Последовательность ходов
int g_hanoi_current_step = -1;
int g_hanoi_start_stick = 0;
int g_hanoi_target_stick = 2;
static int g_hanoi_actual_n = 0;

bool g_hanoi_auto_play = false;
double g_hanoi_last_step_time = 0;
float g_hanoi_speed = 0.6f;  // Секунды между шагами

MutableSegmentedDeque<Ring> g_hanoi_sticks_current[3];

void hanoi_apply_step(int step_idx) {
    HanoiMove m = g_hanoi_moves.get(step_idx);
    Ring r;
    g_hanoi_sticks_current[m.from_stick_index].pop_back(&r);
    g_hanoi_sticks_current[m.to_stick_index].push_back(r);
}

void hanoi_undo_step(int step_idx) {
    HanoiMove m = g_hanoi_moves.get(step_idx);
    Ring r;
    g_hanoi_sticks_current[m.to_stick_index].pop_back(&r);
    g_hanoi_sticks_current[m.from_stick_index].push_back(r);
}

void hanoi_next_step() {
    if (g_hanoi_current_step >= g_hanoi_moves.get_count() - 1) return;
    g_hanoi_current_step++;
    hanoi_apply_step(g_hanoi_current_step);
}

void hanoi_prev_step() {
    if (g_hanoi_current_step < 0) return;
    hanoi_undo_step(g_hanoi_current_step);
    g_hanoi_current_step--;
}

void hanoi_reset_state() {
    for (int idx = 0; idx < 3; idx++) {
        g_hanoi_sticks_current[idx] = MutableSegmentedDeque<Ring>();
    }
    // Сортируем кольца по убыванию размера и кладём на стартовый стержень
    MutableSegmentedDeque<Ring> sorted = g_hanoi_initial_rings;
    sorted.sort([](const Ring& a, const Ring& b) { return a > b; });

    for (int idx = 0; idx < sorted.get_count(); idx++) {
        g_hanoi_sticks_current[g_hanoi_start_stick].push_back(sorted.get(idx));
    }

    g_hanoi_current_step = -1;
}

void draw_deque_window() {
    ImGui::Begin("Segmented Deque");

    ImGui::InputInt("Value", &g_input_value);

    if (ImGui::Button("Push Front")) {
        g_deque.push_front(g_input_value);
    }
    ImGui::SameLine();
    if (ImGui::Button("Push Back")) {
        g_deque.push_back(g_input_value);
    }

    if (ImGui::Button("Pop Front")) {
        if (g_deque.get_count() > 0) {
            int val;
            g_deque.pop_front(&val);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Pop Back")) {
        if (g_deque.get_count() > 0) {
            int val;
            g_deque.pop_back(&val);
        }
    }

    if (ImGui::Button("Reset")) {
        g_deque.reset_deque();
    }

    ImGui::Separator();
    ImGui::Text("Count: %d", g_deque.get_count());

    // Отображение элементов
    ImGui::Text("Elements:");
    for (int idx = 0; idx < g_deque.get_count(); idx++) {
        ImGui::SameLine();
        ImGui::Text("%d", g_deque.get(idx));
    }

    ImGui::Text("Internal structure:");
    render_deque(g_deque);

    ImGui::Separator();

    ImGui::Text("Map");
    ImGui::Combo("Function##map", &g_map_choice, g_map_funcs, 3); // Выпадающий список

    if (ImGui::Button("Apply Map")) {
        Sequence<int>* result = g_deque.map(g_map_handlers[g_map_choice]);

        g_deque = *dynamic_cast<MutableSegmentedDeque<int>*>(result);
        delete result;
    }

    ImGui::Separator();

    ImGui::Text("Where");
    ImGui::Combo("Predicate##where", &g_where_choice, g_where_preds, 3);

    if (ImGui::Button("Apply Where")) {
        Sequence<int>* result = g_deque.where(g_where_handlers[g_where_choice]);
        g_deque = *dynamic_cast<MutableSegmentedDeque<int>*>(result);
        delete result;
    }

    ImGui::Separator();
    ImGui::Text("Sort");

    ImGui::Combo("Order##sort", &g_sort_order, g_sort_orders, 2);

    if (ImGui::Button("Sort")) {
        g_deque.sort(g_sort_handlers[g_sort_order]);
    }

    ImGui::End();
}

void update_benchmark_plot_sizes() {
    for (int idx = 0; idx < g_benchmark_params.sizes_count; idx++) {
        g_benchmark_plot_sizes[idx] = (double)g_benchmark_params.sizes[idx];
    }
}

void draw_benchmark_table(const char* table_id, const char* unit, double* deque_values, double* array_values, double* list_values) {
    char deque_column[32];
    char array_column[32];
    char list_column[32];

    snprintf(deque_column, sizeof(deque_column), "Deque (%s)", unit);
    snprintf(array_column, sizeof(array_column), "Array (%s)", unit);
    snprintf(list_column, sizeof(list_column), "List (%s)", unit);

    if (ImGui::BeginTable(table_id, 4, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn(deque_column);
        ImGui::TableSetupColumn(array_column);
        ImGui::TableSetupColumn(list_column);
        ImGui::TableHeadersRow();

        for (int idx = 0; idx < g_benchmark_params.sizes_count; idx++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", g_benchmark_params.sizes[idx]);
            ImGui::TableNextColumn(); ImGui::Text("%.3f", deque_values[idx]);
            ImGui::TableNextColumn(); ImGui::Text("%.3f", array_values[idx]);
            ImGui::TableNextColumn(); ImGui::Text("%.3f", list_values[idx]);
        }
        ImGui::EndTable();
    }
}

void draw_benchmark_panel(const char* child_id, const char* plot_title, const char* y_axis,
                          const char* table_id, const char* unit,
                          double* deque_values, double* array_values, double* list_values) {

    ImGui::BeginChild(child_id, ImVec2(0, 600), true);

    if (ImPlot::BeginPlot(plot_title)) {
        ImPlot::SetupAxes("Elements count", y_axis);
        ImPlot::PlotLine("SegmentDeque", g_benchmark_plot_sizes, deque_values, g_benchmark_params.sizes_count);
        ImPlot::PlotLine("ArraySequence", g_benchmark_plot_sizes, array_values, g_benchmark_params.sizes_count);
        ImPlot::PlotLine("ListSequence", g_benchmark_plot_sizes, list_values, g_benchmark_params.sizes_count);
        ImPlot::EndPlot();
    }

    draw_benchmark_table(table_id, unit, deque_values, array_values, list_values);
    ImGui::EndChild();
}

void draw_benchmarks_window() {
    ImGui::Begin("Benchmarks");
    update_benchmark_plot_sizes();

    if (ImGui::Button("Run Benchmarks")) {
        run_benchmarks(g_deque_push_front_times, g_array_push_front_times, g_list_push_front_times,
                        g_deque_get_times, g_array_get_times, g_list_get_times,
                        g_mem_deque, g_mem_array, g_mem_list,
                        g_os_deque, g_os_array, g_os_list,
                        g_benchmark_params);

        g_benchmarks_done = true;
    }

    if (g_benchmarks_done) {
        draw_benchmark_panel("PushFrontBenchmark", "Push Front Performance", "Time (ms)",
                             "push_front_bench_table", "ms",
                             g_deque_push_front_times, g_array_push_front_times, g_list_push_front_times);

        draw_benchmark_panel("GetBenchmark", "Get(index) Performance", "Time (ms)",
                             "get_bench_table", "ms",
                             g_deque_get_times, g_array_get_times, g_list_get_times);

        draw_benchmark_panel("MemoryBenchmark", "Memory Performance", "Memory (KB)",
                             "memory_bench_table", "KB",
                             g_mem_deque, g_mem_array, g_mem_list);

        draw_benchmark_panel("OSMemoryBenchmark", "OS Memory Performance", "Memory (KB)",
                             "os_memory_bench_table", "KB",
                             g_os_deque, g_os_array, g_os_list);
    }

    if (ImGui::Button("Stress Test Memory")) {
        size_t before = measure_deque_memory(g_deque);

        // Заполняем
        for (int idx = 0; idx < 10000; idx++) {
            g_deque.push_back(idx);
        }
        for (int idx = 0; idx < 10000; idx++) {
            g_deque.push_front(idx);
        }
        size_t after_push = measure_deque_memory(g_deque);

        // Очищаем
        int val;
        for (int idx = 0; idx < 10000; idx++) {
            g_deque.pop_back(&val);
        }
        for (int idx = 0; idx < 10000; idx++) {
            g_deque.pop_front(&val);
        }
        size_t after_pop = measure_deque_memory(g_deque);

        printf("Before: %zu B, After push: %zu B, After pop: %zu B\n",
            before, after_push, after_pop);
    }

    ImGui::End();
}

void station_generate_random_train() {
    g_station_train = MutableSegmentedDeque<Wagon>();
    int n = 8 + (rand() % 8);

    for (int idx = 0; idx < n; idx++) {
        int type = 1 + (rand() % 3);
        g_station_train.push_back(Wagon(type, idx + 1));
    }

    g_station_steps = MutableSegmentedDeque<StationStep>();
    station_reset_state();
}

void station_run_sort() {
    g_station_steps = MutableSegmentedDeque<StationStep>();
    sort_train_by_type(g_station_train, &g_station_steps);
    station_reset_state();
}

void station_update_auto_play() {
    // Логика автоплея — выполняется каждый кадр
    if (g_station_auto_play) {
        double now = ImGui::GetTime();
        if (now - g_station_last_step_time >= g_station_speed) {
            if (g_station_current_step >= g_station_steps.get_count() - 1) {
                g_station_auto_play = false;
            } else {
                station_next_step();
                g_station_last_step_time = now;
            }
        }
    }
}

void draw_station_wagons(const MutableSegmentedDeque<Wagon>& wagons, ImDrawList* draw, float wagon_weight, float wagon_height, float gap) {
    ImVec2 origin = ImGui::GetCursorScreenPos();

    for (int idx = 0; idx < wagons.get_count(); idx++) {
        ImVec2 pos(origin.x + idx * (wagon_weight + gap), origin.y);
        draw_wagon(draw, pos, wagons.get(idx), wagon_weight, wagon_height);
    }

    ImGui::Dummy(ImVec2(wagons.get_count() * (wagon_weight + gap), wagon_height + 10));
}

void draw_station_current_action() {
    // Текущее действие
    if (g_station_current_step >= 0) {
        StationStep s = g_station_steps.get(g_station_current_step);

        const char* names[] = {"PopFromTrain", "PushToSiding", "PushToResult", "MoveSidingToResult"};
        ImGui::Text("Action: %s - wagon (t%d, %d)", names[(int)s.action], s.wagon.first(), s.wagon.second());
    }
}

void draw_sorting_station_window() {
    // ImGui::SetNextWindowPos(ImVec2(1240, 0), ImGuiCond_Once);
    // ImGui::SetNextWindowSize(ImVec2(900, 700), ImGuiCond_Once);
    ImGui::Begin("Sorting Station");

    if (ImGui::Button("Generate Random Train")) {
        station_generate_random_train();
    }
    ImGui::SameLine();
    if (ImGui::Button("Run Sort")) {
        station_run_sort();
    }

    if (ImGui::Button(g_station_auto_play ? "Pause" : "Auto Play")) {
        g_station_auto_play = !g_station_auto_play;
        g_station_last_step_time = ImGui::GetTime();
    }
    ImGui::SameLine();
    ImGui::SliderFloat("Speed (sec)", &g_station_speed, 0.1f, 2.0f);

    station_update_auto_play();

    ImGui::Separator();

    ImGui::BeginDisabled(g_station_steps.get_count() == 0);
    if (ImGui::Button("< Prev")) station_prev_step();
    ImGui::SameLine();
    if (ImGui::Button("Next >")) station_next_step();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Reset Steps")) station_reset_state();

    ImGui::Text("Step %d / %d", g_station_current_step + 1, g_station_steps.get_count());
    draw_station_current_action();

    ImGui::Separator();

    ImDrawList* draw = ImGui::GetWindowDrawList();
    float wagon_weight = 75, wagon_height = 40, gap = 4;

    ImGui::Text("Train (input):");
    draw_station_wagons(g_station_train_current, draw, wagon_weight, wagon_height, gap);

    ImGui::Text("Sidings:");
    for (int siding_idx = 0; siding_idx < g_station_siding_count; siding_idx++) {
        ImGui::Text("  Siding (t%d):", g_station_types.get(siding_idx));
        draw_station_wagons(g_station_sidings_current[siding_idx], draw, wagon_weight, wagon_height, gap);
    }

    ImGui::Text("Result (output):");
    draw_station_wagons(g_station_result_current, draw, wagon_weight, wagon_height, gap);

    ImGui::End();
}

ImU32 hanoi_color_from_name(const char* name) {
    if (strcmp(name, "red") == 0) return IM_COL32(231, 76, 60, 255);
    if (strcmp(name, "blue") == 0) return IM_COL32(52, 152, 219, 255);
    if (strcmp(name, "green") == 0) return IM_COL32(46, 204, 113, 255);
    if (strcmp(name, "yellow") == 0) return IM_COL32(241, 196, 15, 255);
    if (strcmp(name, "orange") == 0) return IM_COL32(230, 126, 34, 255);
    if (strcmp(name, "purple") == 0) return IM_COL32(155, 89, 182, 255);
    if (strcmp(name, "cyan") == 0) return IM_COL32(26, 188, 156, 255);
    if (strcmp(name, "pink") == 0) return IM_COL32(232, 67, 147, 255);
    if (strcmp(name, "brown") == 0) return IM_COL32(139, 69, 19, 255);
    if (strcmp(name, "gray") == 0) return IM_COL32(127, 140, 141, 255);

    return IM_COL32(150, 150, 150, 255);
}

void hanoi_generate_and_solve(int hanoi_n) {
    g_hanoi_initial_rings = MutableSegmentedDeque<Ring>();
    g_hanoi_actual_n = hanoi_n;
    const char* colors[] = {"red","blue","green","yellow","orange","purple","cyan","pink","brown","gray"};
    const char* shapes[] = {"circle","square","triangle","star","hexagon","diamond","oval","heart","arrow","ring"};

    for (int idx = 0; idx < g_hanoi_actual_n; idx++) {
        g_hanoi_initial_rings.push_back(Ring(idx + 1, (char*)shapes[idx % 10], (char*)colors[idx % 10]));
    }

    g_hanoi_moves = hanoi_collect_moves(g_hanoi_initial_rings,
                                        g_hanoi_start_stick,
                                        g_hanoi_target_stick);
    hanoi_reset_state();
}

void hanoi_update_auto_play() {
    if (g_hanoi_auto_play) {
        double now = ImGui::GetTime();

        if (now - g_hanoi_last_step_time >= g_hanoi_speed) {
            if (g_hanoi_current_step >= g_hanoi_moves.get_count() - 1) {
                g_hanoi_auto_play = false;  // Дошли до конца — выключаем
            } else {
                hanoi_next_step();
                g_hanoi_last_step_time = now;
            }
        }
    }
}

void draw_hanoi_stick(ImDrawList* draw_hanoi, ImVec2 origin, int stick_index,
                      float stick_spacing, float stick_height, float pole_width,
                      float ring_height, float min_ring_w, float max_ring_w) {

    float center_x = origin.x + 100 + stick_index * stick_spacing;
    float base_y = origin.y + stick_height;

    char label[16];
    snprintf(label, sizeof(label), "Stick %d", stick_index);
    draw_hanoi->AddText(ImVec2(center_x - 30, origin.y - 5),
                IM_COL32(255, 255, 255, 255), label);

    draw_hanoi->AddRectFilled(
        ImVec2(center_x - pole_width / 2, origin.y + 20),
        ImVec2(center_x + pole_width / 2, base_y),
        IM_COL32(231, 76, 60, 255), 2.0f);

    draw_hanoi->AddRectFilled(
        ImVec2(center_x - max_ring_w / 2 - 20, base_y),
        ImVec2(center_x + max_ring_w / 2 + 20, base_y + 4),
        IM_COL32(231, 76, 60, 255));

    int n_rings = g_hanoi_sticks_current[stick_index].get_count();
    for (int idx = 0; idx < n_rings; idx++) {
        Ring ring = g_hanoi_sticks_current[stick_index].get(idx);  // i = 0 нижнее

        float w = min_ring_w + (ring.get_size() - 1) *
                (max_ring_w - min_ring_w) / (g_hanoi_actual_n > 1 ? (g_hanoi_actual_n - 1) : 1);

        float ring_y = base_y - (idx + 1) * (ring_height + 2);

        draw_hanoi->AddRectFilled(
            ImVec2(center_x - w / 2, ring_y),
            ImVec2(center_x + w / 2, ring_y + ring_height),
            hanoi_color_from_name(ring.get_color()), ring_height / 2);  // Закругление

        char text[8];
        snprintf(text, sizeof(text), "%d", ring.get_size());
        ImVec2 text_size = ImGui::CalcTextSize(text);
        draw_hanoi->AddText(
            ImVec2(center_x - text_size.x / 2, ring_y + (ring_height - text_size.y) / 2),
            IM_COL32(255, 255, 255, 255), text);
    }
}

void draw_hanoi_tower() {
    ImDrawList* draw_hanoi = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    float stick_spacing = 280.0f;   // Расстояние между стержнями
    float stick_height = 280.0f;    // Высота стержня
    float pole_width = 6.0f;
    float ring_height = 24.0f;
    float min_ring_w = 30.0f;
    float max_ring_w = 200.0f;

    for (int idx = 0; idx < 3; idx++) {
        draw_hanoi_stick(draw_hanoi, origin, idx, stick_spacing, stick_height,
                         pole_width, ring_height, min_ring_w, max_ring_w);
    }

    ImGui::Dummy(ImVec2(3 * stick_spacing, stick_height + 30));
}

void draw_hanoi_window() {
    ImGui::Begin("Hanoi Tower");

    static int hanoi_n = 5;
    ImGui::SliderInt("Rings count", &hanoi_n, 1, 10);
    ImGui::SliderInt("Start stick", &g_hanoi_start_stick, 0, 2);
    ImGui::SliderInt("Target stick", &g_hanoi_target_stick, 0, 2);

    if (g_hanoi_start_stick == g_hanoi_target_stick) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Start and target sticks must be different!");
    } else if (ImGui::Button("Generate & Solve")) {
        hanoi_generate_and_solve(hanoi_n);
    }

    if (ImGui::Button(g_hanoi_auto_play ? "Pause" : "Auto Play")) {
        g_hanoi_auto_play = !g_hanoi_auto_play;
        g_hanoi_last_step_time = ImGui::GetTime();
    }
    ImGui::SameLine();
    ImGui::SliderFloat("Speed (sec)", &g_hanoi_speed, 0.1f, 2.0f);

    hanoi_update_auto_play();

    ImGui::Separator();

    if (ImGui::Button("< Prev")) hanoi_prev_step();
    ImGui::SameLine();
    if (ImGui::Button("Next >")) hanoi_next_step();
    ImGui::SameLine();
    if (ImGui::Button("Reset")) hanoi_reset_state();
    ImGui::SameLine();
    ImGui::Text("Step %d / %d", g_hanoi_current_step + 1, g_hanoi_moves.get_count());

    if (g_hanoi_current_step >= 0 && g_hanoi_current_step < g_hanoi_moves.get_count()) {
        HanoiMove move = g_hanoi_moves.get(g_hanoi_current_step);

        ImGui::Text("Move: ring size=%d (%s) from stick %d to stick %d", move.ring_size, move.ring_color, move.from_stick_index, move.to_stick_index);
    }

    ImGui::Separator();

    draw_hanoi_tower();

    ImGui::End();
}

void draw_gui() {
    draw_deque_window();
    draw_benchmarks_window();
    draw_sorting_station_window();
    draw_hanoi_window();
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Segmented Deque", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(2.0f);  // увеличить виджеты
    io.FontGlobalScale = 2.0f;              // увеличить шрифт
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_gui();

        ImGui::Render();
        int weight, height;
        glfwGetFramebufferSize(window, &weight, &height);
        glViewport(0, 0, weight, height);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
