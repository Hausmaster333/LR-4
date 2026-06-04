#ifndef LAZY_SEQUENCE_VISUALIZER_H
#define LAZY_SEQUENCE_VISUALIZER_H

#include "imgui.h"
#include "lazy/lazy_sequence.h"
#include <cstdio>
#include <algorithm>

inline constexpr int LAZY_MAX_VISIBLE = 64;

inline void draw_lazy_cell(ImDrawList* draw, ImVec2 top_left, float width, float height, ImU32 fill_color, const char* text) {
    ImVec2 bottom_right(top_left.x + width, top_left.y + height);
    draw->AddRectFilled(top_left, bottom_right, fill_color);

    if (text != nullptr && text[0] != '\0') {
        ImVec2 text_size = ImGui::CalcTextSize(text);
        draw->AddText(ImVec2(top_left.x + (width - text_size.x) * 0.5f, top_left.y + (height - text_size.y) * 0.5f), IM_COL32(240, 240, 240, 255), text);
    }
}

inline void print_ordinal(char* buffer, size_t buffer_size, const Ordinal& ordinal) {
    if (ordinal.is_finite()) {
        snprintf(buffer, buffer_size, "%zu", ordinal.get_value());
        return;
    }

    size_t omega_count = ordinal.get_omega_count();
    size_t finite_part = ordinal.get_finite_part();

    if (omega_count == 1 && finite_part == 0) {
        snprintf(buffer, buffer_size, "w");
    } else if (finite_part == 0) {
        snprintf(buffer, buffer_size, "w*%zu", omega_count);
    } else if (omega_count == 1) {
        snprintf(buffer, buffer_size, "w + %zu", finite_part);
    } else {
        snprintf(buffer, buffer_size, "w*%zu + %zu", omega_count, finite_part);
    }
}

// Всё содержимое (append/concat/insert и тд) внутри дерева генератора.
inline void render_lazy_sequence(const LazySequence<int>& sequence) {
    char length_buffer[32];
    print_ordinal(length_buffer, sizeof(length_buffer), sequence.get_length());

    ImGui::Text("Length: %s", length_buffer);
    ImGui::Text("Materialized: %d / %d (sliding cache)", sequence.get_materialized_count(), sequence.get_cache_capacity());

    ImGui::Spacing();

    const float cell_width = 56.0f;
    const float cell_height = 40.0f;
    const float spacing = 2.0f;
    ImDrawList* draw = ImGui::GetWindowDrawList();

    if (sequence.is_cache_empty()) {
        ImGui::TextDisabled("Cache is empty. Press Get/Take to materialize.");
        return;
    }

    size_t first_index = sequence.get_cache_first_index();
    size_t last_index = sequence.get_cache_last_index();
    size_t total = last_index - first_index + 1;
    size_t shown = std::min(total, static_cast<size_t>(LAZY_MAX_VISIBLE));
    size_t start_index = first_index + (total - shown);

    ImGui::TextDisabled("Cache window: [%zu .. %zu]  (showing %zu)", first_index, last_index, shown);

    // Число ячеек в строке подстраивается под ширину окна (как в визуализации памяти):
    // при уменьшении окна ячейки переносятся на новую строку, а не уходят за границу.
    float available_width = ImGui::GetContentRegionAvail().x;
    int per_row = static_cast<int>(available_width / (cell_width + spacing));
    if (per_row < 1) per_row = 1;
    if (per_row > static_cast<int>(shown)) per_row = static_cast<int>(shown);

    ImVec2 origin = ImGui::GetCursorScreenPos();

    for (size_t offset = 0; offset < shown; offset++) {
        size_t logical_index = start_index + offset;
        int row = static_cast<int>(offset) / per_row;
        int col = static_cast<int>(offset) % per_row;

        ImVec2 top_left(origin.x + col * (cell_width + spacing), origin.y + row * (cell_height + spacing));
        char text_buffer[16];
        snprintf(text_buffer, sizeof(text_buffer), "%d", sequence.get_cache_at(logical_index));
        int alpha = 255 - std::min<int>(180, static_cast<int>(offset) * 4);
        ImU32 fill_color = IM_COL32(70, 130, 200, alpha);

        draw_lazy_cell(draw, top_left, cell_width, cell_height, fill_color, text_buffer);
    }

    int rows = (static_cast<int>(shown) + per_row - 1) / per_row;
    ImGui::Dummy(ImVec2(per_row * (cell_width + spacing), rows * (cell_height + spacing)));

    ImGui::TextDisabled("  idx %zu ... idx %zu", start_index, start_index + shown - 1);
}

#endif
