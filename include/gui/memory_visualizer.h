#ifndef MEMORY_VISUALIZER_H
#define MEMORY_VISUALIZER_H

#include "imgui.h"
#include "memory/memory_tape.h"
#include <cstdint>

// Хеш-генератор цвета для block_id. Для -1 (свободно) — темно-серый.
inline ImU32 color_for_block(int block_id) {
    if (block_id < 0) return IM_COL32(50, 55, 65, 255);
    
    uint32_t h = static_cast<uint32_t>(block_id) * 2654435761u;
    int r = 80 + static_cast<int>((h)        & 0x7F);
    int g = 80 + static_cast<int>((h >> 7)   & 0x7F);
    int b = 80 + static_cast<int>((h >> 14)  & 0x7F);
    return IM_COL32(r, g, b, 255);
}

// Рендер ленты памяти квадратиками. Фиксированный размер клетки и фикс.
// Число ячеек в строке — лента не сжимается при росте capacity, окно при необходимости получает горизонтальный скролл от ImGui.
inline void render_memory_tape(const MemoryTape& tape) {
    int cap = tape.get_capacity();
    if (cap <= 0) return;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    const float cell_w  = 40.0f;
    const float cell_h  = 40.0f;
    const float spacing = 2.0f;
    const int per_row = 50;
    const bool draw_id = true;

    for (int i = 0; i < cap; ++i) {
        int row = i / per_row;
        int col = i % per_row;
        ImVec2 p1(origin.x + col * (cell_w + spacing), origin.y + row * (cell_h + spacing));
        ImVec2 p2(p1.x + cell_w, p1.y + cell_h);
        const Cell& c = tape.get_cell(i);
        int block_id = c.used ? c.block_id : -1;
        draw->AddRectFilled(p1, p2, color_for_block(block_id));

        if (draw_id && c.used) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", c.block_id);
            // Точное центрирование через метрики текущего ImGui-шрифта.
            ImVec2 ts = ImGui::CalcTextSize(buf);
            draw->AddText(ImVec2(p1.x + (cell_w - ts.x) * 0.5f, p1.y + (cell_h - ts.y) * 0.5f), IM_COL32(240, 240, 240, 255), buf);
        }
    }

    int rows = (cap + per_row - 1) / per_row;
    ImGui::Dummy(ImVec2(per_row * (cell_w + spacing), rows * (cell_h + spacing)));
}

#endif
