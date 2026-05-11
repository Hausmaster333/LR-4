## Get started:
Clone gtest to your project:
```console
git clone https://github.com/google/googletest.git
```
# GUI
## Build
Windows:
```console
mingw32-make gui
```
Linux:
```console
make gui
```
## Demonstration
<img width="3761" height="1953" alt="image" src="https://github.com/user-attachments/assets/f7fdd63c-d80a-434d-a693-8fccd46fb257" />

### Used libraries
Для графического интерфейса используются:

- Dear ImGui (vendor/imgui) — immediate mode GUI-библиотека для отрисовки окон, кнопок, таблиц, слайдеров и других элементов интерфейса.
- ImPlot (vendor/implot) — библиотека для построения графиков внутри Dear ImGui. Используется для отображения результатов benchmark-тестов.
- GLFW — библиотека для создания окна, обработки событий и OpenGL-контекста.
- OpenGL — используется как графический backend для Dear ImGui.
  
Все исходники ImGui и ImPlot лежат в `vendor/`, поэтому отдельно скачивать их не нужно. 
## Build CLI
Windows:
```console
mingw32-make program
```
Linux:
```console
make program
```
## Build tests
Windows:
```console
mingw32-make deq_tests
```
Linux:
```console
make deq_tests
```

## Demonstration of Hanoi tower using HTML:

<video src="https://github.com/user-attachments/assets/0ca40a36-81dd-4d00-825f-1d616676ed22" controls></video>
