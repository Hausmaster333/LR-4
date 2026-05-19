#ifndef LAZY_SEQUENCE_VISUALIZER_H
#define LAZY_SEQUENCE_VISUALIZER_H

#include "imgui.h"
#include "lazy/lazy_sequence.h"
#include <cstdio>
#include <algorithm>

// Максимум ячеек в одном ряду визуализатора (для очень больших окон)
inline constexpr int LAZY_MAX_VISIBLE = 64;

// Рисует одну ячейку с центрированным текстом внутри
inline void draw_lazy_cell(ImDrawList* draw, ImVec2 top_left, float width, float height, ImU32 fill_color, const char* text) {
    ImVec2 bottom_right(top_left.x + width, top_left.y + height);
    draw->AddRectFilled(top_left, bottom_right, fill_color);

    if (text != nullptr && text[0] != '\0') {
        ImVec2 text_size = ImGui::CalcTextSize(text);
        draw->AddText(ImVec2(top_left.x + (width - text_size.x) * 0.5f, top_left.y + (height - text_size.y) * 0.5f), IM_COL32(240, 240, 240, 255), text);
    }
}

// Печатает Ordinal: либо «infinity», либо число
inline void print_cardinal(char* buffer, size_t buffer_size, const Ordinal& cardinal) {
    if (cardinal.is_infinite()) {
        snprintf(buffer, buffer_size, "infinity");
    } else {
        snprintf(buffer, buffer_size, "%zu", cardinal.get_value());
    }
}

// Рендер LazySequence - статистика + текущее окно кэша + tail-ряд
inline void render_lazy_sequence(const LazySequence<int>& sequence) {
    char length_buffer[32], base_buffer[32], tail_buffer[32];
    print_cardinal(length_buffer, sizeof(length_buffer), sequence.get_length());
    print_cardinal(base_buffer, sizeof(base_buffer), sequence.get_base_length());
    print_cardinal(tail_buffer, sizeof(tail_buffer), sequence.get_tail_added_length());

    ImGui::Text("Length: %s", length_buffer);
    ImGui::Text("Base length: %s", base_buffer);
    ImGui::Text("Tail added length: %s (ops in queue: %d)", tail_buffer, sequence.get_tail_op_count());
    ImGui::Text("Materialized: %d / %d (sliding cache)",
                sequence.get_materialized_count(), sequence.get_cache_capacity());

    ImGui::Spacing();

    const float cell_width = 56.0f;
    const float cell_height = 40.0f;
    const float spacing = 2.0f;
    ImDrawList* draw = ImGui::GetWindowDrawList();

    if (sequence.is_cache_empty()) {
        ImGui::TextDisabled("Cache is empty. Press Get/Take to materialize.");
    } else {
        size_t first_index = sequence.get_cache_first_index();
        size_t last_index = sequence.get_cache_last_index();
        size_t total = last_index - first_index + 1;
        size_t shown = std::min(total, static_cast<size_t>(LAZY_MAX_VISIBLE));
        // Если окно больше лимита - показываем последние LAZY_MAX_VISIBLE
        size_t start_index = first_index + (total - shown);

        ImGui::TextDisabled("Cache window: [%zu .. %zu]  (showing %zu)", first_index, last_index, shown);

        ImVec2 origin = ImGui::GetCursorScreenPos();
        for (size_t offset = 0; offset < shown; offset++) {
            size_t logical_index = start_index + offset;
            ImVec2 top_left(origin.x + offset * (cell_width + spacing), origin.y);
            char text_buffer[16];
            snprintf(text_buffer, sizeof(text_buffer), "%d", sequence.get_cache_at(logical_index));
            // Цвет - оттенок синего
            int alpha = 255 - std::min<int>(180, static_cast<int>(offset) * 4);
            ImU32 fill_color = IM_COL32(70, 130, 200, alpha);
            draw_lazy_cell(draw, top_left, cell_width, cell_height, fill_color, text_buffer);
        }
        ImGui::Dummy(ImVec2(shown * (cell_width + spacing), cell_height));

        ImGui::TextDisabled("  idx %zu ... idx %zu", start_index, start_index + shown - 1);
    }

    int tail_added = sequence.get_tail_added_length().is_finite() ? static_cast<int>(sequence.get_tail_added_length().get_value()) : 0;
    if (tail_added > 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("Deferred tail values (queued ops: %d, total: %d):", sequence.get_tail_op_count(), tail_added);

        const float tail_cell_width = 56.0f;
        const float tail_cell_height = 32.0f;
        int shown_tail = std::min(tail_added, LAZY_MAX_VISIBLE);
        ImVec2 tail_origin = ImGui::GetCursorScreenPos();

        for (int index = 0; index < shown_tail; index++) {
            ImVec2 top_left(tail_origin.x + index * (tail_cell_width + spacing), tail_origin.y);
            char text_buffer[16];
            snprintf(text_buffer, sizeof(text_buffer), "%d", sequence.get_tail_at(index));
            draw_lazy_cell(draw, top_left, tail_cell_width, tail_cell_height,
                           IM_COL32(220, 140, 40, 255), text_buffer);
        }

        ImGui::Dummy(ImVec2(shown_tail * (tail_cell_width + spacing), tail_cell_height));
    }
}

#endif
