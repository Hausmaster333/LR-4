#ifndef DEQUE_VISUALIZER_H
#define DEQUE_VISUALIZER_H

#include "imgui.h"
#include "deque/segment_deque.h"
#include <chrono>

bool check_is_active_cell(int checked_block, int checked_index, int front_block, int front_index, int back_block, int back_index, int segment_size) {
    int total_checked_index = checked_block * segment_size + checked_index;
    int total_front_index = front_block * segment_size + front_index;
    int total_back_index = back_block * segment_size + back_index;

    return (total_checked_index >= total_front_index && total_checked_index < total_back_index);
};

template <class U>
void render_deque(const SegmentedDeque<U>& deque) {
    ImGui::Text("Front block: %d, index: %d", deque.front_block, deque.front_index);
    ImGui::Text("Back block: %d, index: %d", deque.back_block, deque.back_index);
    ImGui::Text("Map capacity: %d", deque.map_capacity);
    ImGui::Text("Count: %d", deque.count);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    float cell_size = 40.0f;
    float spacing = 4.0f;

    for (int curr_block_index = 0; curr_block_index < deque.map_capacity; curr_block_index++) {
        U* curr_block = deque.block_map.get(curr_block_index);
        float block_y = origin.y + curr_block_index * (cell_size + spacing);

        for (int curr_cell_index = 0; curr_cell_index < deque.segment_size; curr_cell_index++) {
            float x = origin.x + curr_cell_index * (cell_size + spacing);
            float y = block_y;

            ImVec2 p1(x, y);
            ImVec2 p2(x + cell_size, y + cell_size);

            if (curr_block == nullptr) {
                draw->AddRect(p1, p2, IM_COL32(100, 100, 100, 100));
            } else {
                bool is_active = check_is_active_cell(curr_block_index, curr_cell_index, deque.front_block, deque.front_index, deque.back_block, deque.back_index, deque.segment_size);

                if (is_active) {
                    draw->AddRectFilled(p1, p2, IM_COL32(100, 200, 100, 255));

                    char text[16];
                    snprintf(text, sizeof(text), "%d", (int)curr_block[curr_cell_index]);
                    draw->AddText(ImVec2(x + 10, y + 10), IM_COL32(255, 255, 255, 255), text);
                } else {
                    draw->AddRectFilled(p1, p2, IM_COL32(60, 60, 80, 255));
                }
            }
        }
    }

    ImGui::Dummy(ImVec2(deque.segment_size * (cell_size + spacing), deque.map_capacity * (cell_size + spacing)));
};

#endif
