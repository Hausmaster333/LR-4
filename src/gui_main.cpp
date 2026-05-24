#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "gui/memory_visualizer.h"
#include "gui/lazy_sequence_visualizer.h"
#include "memory/memory_tape.h"
#include "memory/alloc_event.h"
#include "memory/alloc_event_stream.h"
#include "streams/lazy_read_stream.h"
#include "lazy/lazy_sequence.h"
#include "lazy/sliding_cache.h"
#include "core/sequence.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <iostream>

// ============================

static int g_mem_capacity = 64;
static int g_mem_alloc_size = 3;
static int g_mem_manual_free_id = 0;
static int g_mem_strategy_idx = 0;
static const char* g_mem_strategies[] = {"First-fit", "Best-fit", "Worst-fit"};
static int g_mem_seed_input = 12345;

static MemoryTape* g_mem_tape = nullptr;
static LazySequence<AllocEvent>* g_mem_stream = nullptr;
static LazyReadStream<AllocEvent>* g_mem_reader = nullptr;

struct MemLogEntry { char text[96]; };
static const int g_mem_log_max = 200;
static SlidingCache<MemLogEntry> g_mem_log(g_mem_log_max);

void memory_log(const char* fmt, ...) {
    MemLogEntry entry;
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.text, sizeof(entry.text), fmt, args);
    va_end(args);

    g_mem_log.push(entry);
}

void memory_reset() {
    delete g_mem_reader;
    delete g_mem_stream;
    delete g_mem_tape;

    g_mem_reader = nullptr;
    g_mem_stream = nullptr;
    g_mem_tape = nullptr;
    g_mem_log.clear();

    if (g_mem_capacity < 1) g_mem_capacity = 1;
    if (g_mem_capacity > 4096) g_mem_capacity = 4096;

    g_mem_tape = new MemoryTape(g_mem_capacity);
    g_mem_stream = make_alloc_event_stream(static_cast<uint64_t>(g_mem_seed_input), 32, 5, 70);
    g_mem_reader = new LazyReadStream<AllocEvent>(g_mem_stream);
    g_mem_reader->open();
}

void memory_init_if_needed() {
    if (g_mem_tape == nullptr) memory_reset();
}

void memory_apply_event(const AllocEvent& event) {
    AllocStrategy strategy = static_cast<AllocStrategy>(g_mem_strategy_idx);

    if (event.kind == AllocEventKind::Alloc) {
        int new_id = g_mem_tape->alloc(event.payload, strategy);
        memory_log("Stream Alloc(size=%d) -> id=%d", event.payload, new_id);
        return;
    }

    // Стрим не знает, какие id реально живы — он выдаёт «псевдослучайный»
    // payload из [0..existing_blocks_cap). Маппим его на один из реально
    // живых блоков ленты, чтобы не получать бесконечные miss-ы
    int capacity = g_mem_tape->get_capacity();
    int live_count = 0;
    int last_seen = -1;
    for (int index = 0; index < capacity; index++) {
        const Cell& cell = g_mem_tape->get_cell(index);

        if (!cell.used) continue;
        if (cell.block_id == last_seen) continue;

        last_seen = cell.block_id;
        live_count++;
    }
    if (live_count == 0) {
        memory_log("Stream Free -> skipped (no live blocks)");
        return;
    }

    int target = event.payload % live_count;
    int passed = 0;
    int live_id = -1;
    last_seen = -1;
    for (int index = 0; index < capacity && live_id < 0; index++) {
        const Cell& cell = g_mem_tape->get_cell(index);

        if (!cell.used) continue;
        if (cell.block_id == last_seen) continue;

        last_seen = cell.block_id;

        if (passed == target) live_id = cell.block_id;
        passed++;
    }

    bool ok = g_mem_tape->free(live_id);
    memory_log("Stream Free(id=%d) -> %s", live_id, ok ? "ok" : "miss");
}

void memory_step_from_stream() {
    if (g_mem_reader == nullptr) return;

    if (g_mem_reader->is_end_of_stream()) {
        memory_log("Stream: end of stream");
        return;
    }

    AllocEvent event = g_mem_reader->read();
    memory_apply_event(event);
}

// Доводит ленту до максимально фрагментированного состояния:
// заливает все ячейки блоками по 1, затем освобождает каждый чётный id
// Итог - узор [U F U F U F …], frag → 1.0, любой alloc(>=2) даст -1
// несмотря на ~50% свободного места
void memory_fragment_chaos() {
    g_mem_tape->reset();
    int capacity = g_mem_tape->get_capacity();
    AllocStrategy strategy = static_cast<AllocStrategy>(g_mem_strategy_idx);

    for (int index = 0; index < capacity; index++) {
        g_mem_tape->alloc(1, strategy);
    }

    int half_freed = 0;

    for (int block_id = 0; block_id < capacity; block_id += 2) {
        if (g_mem_tape->free(block_id)) half_freed++;
    }

    memory_log("CHAOS: filled %d cells, freed %d alternates", capacity, half_freed);
    memory_log("CHAOS: fragmentation = %.1f%%", g_mem_tape->fragmentation() * 100.0);
}

void draw_memory_window() {
    memory_init_if_needed();
    ImGui::Begin("Memory Tape Allocator");

    bool need_reset = false;

    if (ImGui::InputInt("Capacity", &g_mem_capacity)) need_reset = true;

    ImGui::Combo("Strategy", &g_mem_strategy_idx, g_mem_strategies, 3);
    ImGui::SliderInt("Alloc size", &g_mem_alloc_size, 1, 16);
    ImGui::InputInt("Free block id", &g_mem_manual_free_id);
    ImGui::InputInt("Seed", &g_mem_seed_input);

    AllocStrategy strategy = static_cast<AllocStrategy>(g_mem_strategy_idx);

    if (ImGui::Button("Manual Alloc")) {
        int new_id = g_mem_tape->alloc(g_mem_alloc_size, strategy);
        memory_log("Manual Alloc(size=%d) -> id=%d", g_mem_alloc_size, new_id);
    }
    ImGui::SameLine();
    if (ImGui::Button("Manual Free")) {
        bool ok = g_mem_tape->free(g_mem_manual_free_id);
        memory_log("Manual Free(id=%d) -> %s", g_mem_manual_free_id, ok ? "ok" : "miss");
    }
    ImGui::SameLine();
    if (ImGui::Button("Step from Stream")) memory_step_from_stream();
    ImGui::SameLine();
    if (ImGui::Button("Step x10")) {
        for (int step = 0; step < 10; step++) memory_step_from_stream();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) need_reset = true;

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(160, 30, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(200, 40, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(220, 60, 60, 255));
    bool chaos_clicked = ImGui::Button("DO NOT PRESS UNDER ANY CIRCUMSTANCES");
    ImGui::PopStyleColor(3);
    if (chaos_clicked) memory_fragment_chaos();

    if (need_reset) memory_reset();

    ImGui::Separator();
    ImGui::Text("Used: %d / %d   Free: %d   Blocks: %d   Frag: %.1f%%",
                g_mem_tape->get_used_count(),
                g_mem_tape->get_capacity(),
                g_mem_tape->get_free_count(),
                g_mem_tape->get_blocks_count(),
                g_mem_tape->fragmentation() * 100.0);
    ImGui::Separator();

    render_memory_tape(*g_mem_tape);

    ImGui::Separator();
    ImGui::Text("Recent events (last 20):");
    if (!g_mem_log.is_empty()) {
        size_t last = g_mem_log.get_last_index();
        size_t first = g_mem_log.get_first_index();
        size_t shown = std::min<size_t>(20, last - first + 1);
        for (size_t i = last - shown + 1; i <= last; i++) {
            ImGui::TextUnformatted(g_mem_log.get(i).text);
        }
    }

    ImGui::End();
}

// ============================

static LazySequence<int>* g_lazy_seq = nullptr;
static int g_lazy_source_idx = 0;
static int g_lazy_get_index = 0;
static int g_lazy_take_n = 10;
static int g_lazy_value = 7;
static int g_lazy_map_choice = 0;
static int g_lazy_where_choice = 0;
static int g_lazy_cache_capacity = 64;
static const char* g_lazy_source_names[] = {"Natural Numbers", "Fibonacci", "Powers of 2", "Finite {1..5}"};
static const char* g_lazy_map_names[] = {"x * 2", "x + 1", "-x"};
static const char* g_lazy_where_names[] = {"even", "odd", "x > 0"};

struct LazyLogEntry { char text[96]; };
static const int g_lazy_log_max = 200;
static SlidingCache<LazyLogEntry> g_lazy_log(g_lazy_log_max);

void lazy_log(const char* fmt, ...) {
    LazyLogEntry entry;
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.text, sizeof(entry.text), fmt, args);
    va_end(args);
    g_lazy_log.push(entry);
}

void lazy_reset(int source_idx) {
    delete g_lazy_seq;
    g_lazy_seq = nullptr;
    g_lazy_log.clear();

    if (g_lazy_cache_capacity < 2) g_lazy_cache_capacity = 2;
    if (g_lazy_cache_capacity > 65536) g_lazy_cache_capacity = 65536;
    int capacity = g_lazy_cache_capacity;

    switch (source_idx) {
        case 0: { // Natural Numbers
            MutableArraySequence<int> initial;
            initial.append(0);

            std::function<int(Sequence<int>*)> rule = [](Sequence<int>* window) {
                return window->get_last() + 1;
            };

            g_lazy_seq = new LazySequence<int>(rule, &initial, capacity);
            break;
        }
        case 1: { // Fibonacci
            MutableArraySequence<int> initial;
            initial.append(0);
            initial.append(1);

            std::function<int(Sequence<int>*)> rule = [](Sequence<int>* window) {
                return window->get_first() + window->get_last();
            };

            g_lazy_seq = new LazySequence<int>(rule, &initial, capacity);
            break;
        }
        case 2: { // Powers of 2: 1,2,4,8,...
            MutableArraySequence<int> initial;
            initial.append(1);

            std::function<int(Sequence<int>*)> rule = [](Sequence<int>* window) {
                return window->get_last() * 2;
            };

            g_lazy_seq = new LazySequence<int>(rule, &initial, capacity);
            break;
        }
        case 3: { // Finite {1..5}
            MutableArraySequence<int> source;

            for (int value = 1; value <= 5; value++) {
                source.append(value);
            }

            g_lazy_seq = new LazySequence<int>(&source, capacity);
            break;
        }
    }
    lazy_log("source = %s (cache cap = %d)", g_lazy_source_names[source_idx], capacity);
}

void lazy_replace(LazySequence<int>* fresh, const char* note) {
    delete g_lazy_seq;
    g_lazy_seq = fresh;
    lazy_log("%s -> ok", note);
}

void lazy_init_if_needed() {
    if (g_lazy_seq == nullptr) lazy_reset(g_lazy_source_idx);
}

void draw_lazy_window() {
    lazy_init_if_needed();
    ImGui::Begin("Lazy Sequence");

    if (ImGui::Combo("Source", &g_lazy_source_idx, g_lazy_source_names, 4)) {
        lazy_reset(g_lazy_source_idx);
    }
    ImGui::SetNextItemWidth(150);

    ImGui::InputInt("Cache capacity", &g_lazy_cache_capacity);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) lazy_reset(g_lazy_source_idx);

    ImGui::Separator();

    // get
    ImGui::SetNextItemWidth(150);
    ImGui::InputInt("##get_i", &g_lazy_get_index);
    ImGui::SameLine();
    if (ImGui::Button("Get")) {
        try {
            int value = g_lazy_seq->get(g_lazy_get_index);
            lazy_log("get(%d) = %d", g_lazy_get_index, value);
        } catch (const std::exception& ex) {
            lazy_log("get(%d) threw: %s", g_lazy_get_index, ex.what());
        }
    }

    // take
    ImGui::SetNextItemWidth(150);
    ImGui::InputInt("##take_n", &g_lazy_take_n);
    ImGui::SameLine();
    if (ImGui::Button("Take")) {
        try {
            lazy_replace(g_lazy_seq->take(g_lazy_take_n), "take");
        } catch (const std::exception& ex) {
            lazy_log("take threw: %s", ex.what());
        }
    }

    // append/prepend value
    ImGui::SetNextItemWidth(150);
    ImGui::InputInt("##val", &g_lazy_value);

    ImGui::SameLine();
    if (ImGui::Button("Append")) lazy_replace(g_lazy_seq->append(g_lazy_value), "append");

    ImGui::SameLine();
    if (ImGui::Button("Prepend")) lazy_replace(g_lazy_seq->prepend(g_lazy_value), "prepend");

    // concat
    if (ImGui::Button("Concat {100,200,300}")) {
        MutableArraySequence<int> right_side;

        right_side.append(100);
        right_side.append(200);
        right_side.append(300);

        LazySequence<int>* other = new LazySequence<int>(&right_side);
        try {
            lazy_replace(g_lazy_seq->concat(other), "concat");
        } catch (const std::exception& ex) {
            lazy_log("concat threw: %s", ex.what());
        }
        delete other;
    }

    // map
    ImGui::SetNextItemWidth(150);
    ImGui::Combo("Map fn", &g_lazy_map_choice, g_lazy_map_names, 3);
    ImGui::SameLine();
    if (ImGui::Button("Apply Map")) {
        std::function<int(const int&)> function;

        if (g_lazy_map_choice == 0) function = [](const int& x) { return x * 2; };
        else if (g_lazy_map_choice == 1) function = [](const int& x) { return x + 1; };
        else function = [](const int& x) { return -x; };

        try {
            lazy_replace(g_lazy_seq->map<int>(function), "map");
        } catch (const std::exception& ex) {
            lazy_log("map threw: %s", ex.what());
        }
    }

    // where
    ImGui::SetNextItemWidth(150);
    ImGui::Combo("Where pred", &g_lazy_where_choice, g_lazy_where_names, 3);
    ImGui::SameLine();
    if (ImGui::Button("Apply Where")) {
        std::function<bool(const int&)> predicate;

        if (g_lazy_where_choice == 0) predicate = [](const int& x) { return x % 2 == 0; };
        else if (g_lazy_where_choice == 1) predicate = [](const int& x) { return x % 2 != 0; };
        else predicate = [](const int& x) { return x > 0; };

        try {
            lazy_replace(g_lazy_seq->where(predicate), "where");
        } catch (const std::exception& ex) {
            lazy_log("where threw: %s", ex.what());
        }
    }

    // zip with naturals (sum)
    if (ImGui::Button("Zip with Naturals (sum)")) {
        MutableArraySequence<int> initial;
        initial.append(0);

        std::function<int(Sequence<int>*)> rule = [](Sequence<int>* window) {
            return window->get_last() + 1;
        };

        LazySequence<int>* naturals = new LazySequence<int>(rule, &initial);

        std::function<int(const int&, const int&)> add = [](const int& a, const int& b) {
            return a + b;
        };

        try {
            lazy_replace(g_lazy_seq->zip<int, int>(naturals, add), "zip");
        } catch (const std::exception& ex) {
            lazy_log("zip threw: %s", ex.what());
        }
        delete naturals;
    }

    // Кнопка материализации первых N 
    ImGui::SameLine();
    if (ImGui::Button("Materialize first 32")) {
        int limit = 32;
        Ordinal length = g_lazy_seq->get_length();

        if (length.is_finite() && length.get_value() < static_cast<size_t>(limit)) {
            limit = static_cast<int>(length.get_value());
        }

        try {
            for (int index = 0; index < limit; index++) g_lazy_seq->get(index);
            lazy_log("materialized 0..%d", limit - 1);
        } catch (const std::exception& ex) {
            lazy_log("materialize threw: %s", ex.what());
        }
    }

    ImGui::Separator();
    render_lazy_sequence(*g_lazy_seq);

    ImGui::Separator();
    ImGui::Text("Recent operations (last 20):");
    if (!g_lazy_log.is_empty()) {
        size_t last = g_lazy_log.get_last_index();
        size_t first = g_lazy_log.get_first_index();
        size_t shown = std::min<size_t>(20, last - first + 1);

        for (size_t i = last - shown + 1; i <= last; i++) {
            ImGui::TextUnformatted(g_lazy_log.get(i).text);
        }

    }

    ImGui::End();
}

// ===========================

void draw_gui() {
    draw_memory_window();
    draw_lazy_window();
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "LR-4 GUI", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    float xscale = 1.5f, yscale = 1.5f;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor != nullptr) glfwGetMonitorContentScale(monitor, &xscale, &yscale);
    float dpi_scale = (xscale > 0.0f) ? xscale : 3.0f;
    ImGui::GetStyle().ScaleAllSizes(dpi_scale);
    io.FontGlobalScale = dpi_scale;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_gui();

        ImGui::Render();
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
