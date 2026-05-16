# mingw32-make для сборки на Windows
# make для сборки на Linux
CC = g++
CFLAGS = -Wall -std=c++17 -Iinclude # Все предупреждения компилятора

IMGUI_DIR = vendor/imgui
IMPLOT_DIR = vendor/implot
GUI_FLAGS = -I$(IMGUI_DIR) -I$(IMPLOT_DIR)

IMGUI_SRC = $(IMGUI_DIR)/imgui.cpp \
            $(IMGUI_DIR)/imgui_draw.cpp \
            $(IMGUI_DIR)/imgui_tables.cpp \
            $(IMGUI_DIR)/imgui_widgets.cpp \
            $(IMGUI_DIR)/imgui_demo.cpp \
            $(IMGUI_DIR)/imgui_impl_glfw.cpp \
            $(IMGUI_DIR)/imgui_impl_opengl3.cpp

IMPLOT_SRC = $(IMPLOT_DIR)/implot.cpp \
             $(IMPLOT_DIR)/implot_items.cpp

ifeq ($(OS),Windows_NT)
    GUI_LIBS = -lglfw3 -lopengl32 -lgdi32 -limm32
else
    GUI_LIBS = -lglfw -lGL -ldl -lpthread -lX11
endif

GTEST_DIR = googletest/googletest
GTEST_FLAGS = -I$(GTEST_DIR)/include -I$(GTEST_DIR)

program: src/main.cpp src/menu.cpp src/bit_sequence.cpp 
	$(CC) $(CFLAGS) src/main.cpp src/menu.cpp src/bit_sequence.cpp -o program

# GUI версия с ImGui+ImPlot
gui: src/gui_main.cpp $(IMGUI_SRC) $(IMPLOT_SRC)
	$(CC) $(CFLAGS) $(GUI_FLAGS) src/gui_main.cpp $(IMGUI_SRC) $(IMPLOT_SRC) $(GUI_LIBS) -o gui

seq_tests: tests/tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc
	$(CC) $(CFLAGS) $(GTEST_FLAGS) tests/tests.cpp src/bit_sequence.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc -o seq_tests

deq_tests: tests/deque_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc src/sorting_station.cpp
	$(CC) $(CFLAGS) $(GTEST_FLAGS) tests/deque_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc src/sorting_station.cpp -o deq_tests

deq_tests_leak: tests/deque_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc
	$(CC) $(CFLAGS) -g -O0 $(GTEST_FLAGS) tests/deque_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc -o deq_tests_leak
#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./deq_tests_leak

lazy_tests: tests/lazy_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc
	$(CC) $(CFLAGS) $(GTEST_FLAGS) tests/lazy_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc -o lazy_tests

stream_tests: tests/stream_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc
	$(CC) $(CFLAGS) $(GTEST_FLAGS) tests/stream_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc -o stream_tests

memory_tape_tests: tests/memory_tape_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc
	$(CC) $(CFLAGS) $(GTEST_FLAGS) tests/memory_tape_tests.cpp $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc -o memory_tape_tests

clean:
	rm *.o program seq_tests stream_tests memory_tape_tests
