#ifndef MEMORY_VISUALIZER_H
#define MEMORY_VISUALIZER_H

#include "imgui.h"
#include "memory/memory_tape.h"
#include <cstdint>

// Хеш-генератор цвета для block_id. Для -1 (свободно) — темно-серый.
inline ImU32 color_for_block(int block_id) {
    if (block_id < 0) return IM_COL32(50, 55, 65, 255);
    
    uint32_t hash = static_cast<uint32_t>(block_id) * 2654435761u;
    int red   = 80 + static_cast<int>((hash)        & 0x7F);
    int green = 80 + static_cast<int>((hash >> 7)   & 0x7F);
    int blue  = 80 + static_cast<int>((hash >> 14)  & 0x7F);
    return IM_COL32(red, green, blue, 255);
}

// Рендер ленты памяти квадратиками. Число ячеек в строке (per_row) подстраивается
// под текущую ширину окна - при уменьшении окна лента переносит ячейки на новую
// строку вместо ухода за границу.
inline void render_memory_tape(const MemoryTape& tape) {
    int cap = tape.get_capacity();
    if (cap <= 0) return;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    const float cell_w  = 40.0f;
    const float cell_h  = 40.0f;
    const float spacing = 2.0f;
    const bool draw_id = true;

    float available_width = ImGui::GetContentRegionAvail().x;
    int per_row = static_cast<int>(available_width / (cell_w + spacing));

    if (per_row < 1) per_row = 1;
    if (per_row > cap) per_row = cap;

    for (int i = 0; i < cap; ++i) {
        int row = i / per_row;
        int col = i % per_row;
        ImVec2 p1(origin.x + col * (cell_w + spacing), origin.y + row * (cell_h + spacing));
        ImVec2 p2(p1.x + cell_w, p1.y + cell_h);
        const Cell& cell = tape.get_cell(i);
        int block_id = cell.used ? cell.block_id : -1;
        draw->AddRectFilled(p1, p2, color_for_block(block_id));

        if (draw_id && cell.used) {
            char text_buffer[8];
            snprintf(text_buffer, sizeof(text_buffer), "%d", cell.block_id);
            ImVec2 text_size = ImGui::CalcTextSize(text_buffer);
            draw->AddText(ImVec2(p1.x + (cell_w - text_size.x) * 0.5f, p1.y + (cell_h - text_size.y) * 0.5f), IM_COL32(240, 240, 240, 255), text_buffer);
        }
    }

    int rows = (cap + per_row - 1) / per_row;
    ImGui::Dummy(ImVec2(per_row * (cell_w + spacing), rows * (cell_h + spacing)));
}

#endif
