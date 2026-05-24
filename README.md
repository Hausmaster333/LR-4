# LR-4: Ленивые последовательности с трансфинитными операциями

Лабораторная работа по структурам данных. Реализация ленивых последовательностей (`LazySequence<T>`) с поддержкой бесконечных потоков и ординальной арифметики (`ω·k + n`), а также симулятор ленты памяти с аллокатором (First-fit / Best-fit / Worst-fit).

## Основные возможности

- **Бесконечные последовательности** — рекуррентные генераторы (натуральные числа, Фибоначчи, степени двойки и др.)
- **Ординальная арифметика** — некоммутативное сложение/вычитание (`1 + ω = ω`, но `ω + 1 = ω + 1`), ординальные индексы для доступа к элементам за `ω`-границей
- **Композиция** — `concat`, `insert_at`, `map`, `where`, `zip`, `take`, `append`, `prepend`
- **Sliding cache** — кольцевой кэш последних материализованных значений
- **Потоковый ввод/вывод** — `ReadOnlyStream` / `WriteOnlyStream` над файлами, строками, `Sequence` и `LazySequence`
- **Memory Tape** — симулятор непрерывной памяти с аллокацией/освобождением блоков
- **GUI** — визуализация ленты памяти и ленивой последовательности через Dear ImGui

## Быстрый старт

### Зависимости

```console
git clone https://github.com/google/googletest.git
```

Dear ImGui и GLFW должны быть доступны (исходники ImGui лежат в `vendor/imgui`).

### Сборка GUI

Windows:
```console
mingw32-make gui
```

Linux:
```console
make gui
```

### Сборка тестов

Windows:
```console
mingw32-make lazy_tests
mingw32-make ordinal_tests
mingw32-make stream_tests
mingw32-make memory_tape_tests
```

Linux — аналогично через `make`.

### Запуск тестов

```console
./lazy_tests
./ordinal_tests
./stream_tests
./memory_tape_tests
```

## GUI

### Демонстрация

<img width="3761" height="1953" alt="image" src="https://github.com/user-attachments/assets/f7fdd63c-d80a-434d-a693-8fccd46fb257" />

### Memory Tape Allocator

Симулятор ленты памяти фиксированной длины. Поддерживает три стратегии аллокации:

- **First-fit** — первый подходящий свободный блок
- **Best-fit** — минимальный подходящий блок
- **Worst-fit** — максимальный подходящий блок

Поток событий (`Alloc`/`Free`) генерируется бесконечным `AllocEventGenerator` на основе LCG (линейный конгруэнтный генератор). Визуализация ленты — цветные квадратики, цвет определяется хешем `block_id`.

### Lazy Sequence

Панель для работы с ленивыми последовательностями. Доступные источники:

- Natural Numbers (`0, 1, 2, ...`)
- Fibonacci (`1, 1, 2, 3, 5, 8, ...`)
- Powers of 2 (`1, 2, 4, 8, ...`)
- Finite `{1, 2, 3, 4, 5}`

Кнопки: `Get`, `Take`, `Append`, `Prepend`, `Concat`, `Map`, `Where`, `Zip`, `Materialize`. Каждая операция возвращает новую `LazySequence` (иммутабельный API). Визуализатор показывает длину в ординальном формате, состояние sliding-cache и последние материализованные значения.

### Используемые библиотеки

- **Dear ImGui** (`vendor/imgui`) — immediate mode GUI для отрисовки окон, кнопок, таблиц и слайдеров
- **GLFW** — создание окна, обработка событий и OpenGL-контекст
- **OpenGL** — графический backend для Dear ImGui

Все исходники ImGui лежат в `vendor/`, отдельно скачивать не нужно.

### Ключевые модули

```
include/
├── core/           Базовые контейнеры (Sequence, DynamicArray, LinkedList, Option)
├── lazy/           Ленивые последовательности и ординальная арифметика
│   ├── ordinal.h           Ordinal: ω * n + m, некоммутативная арифметика
│   ├── generator.h/.tpp    Generator<T> и все наследники
│   ├── lazy_sequence.h/.tpp  LazySequence<T>
│   └── sliding_cache.h     Кольцевой кэш
├── streams/        Потоковый ввод/вывод (файлы, строки, Sequence, LazySequence)
├── memory/         Memory Tape + AllocEventGenerator
└── gui/            Визуализаторы для ImGui
```

### Иерархия генераторов

| Генератор | Назначение | OrdinalIndexable |
|---|---|---|
| `SourceGenerator` | Обёртка над готовым `Sequence` (финитный буфер) | да |
| `RecurrenceGenerator` | Рекуррентное правило `f(window) → next` | нет |
| `PrependGenerator` | Элемент + upstream | да |
| `InsertAtGenerator` | upstream[0..p) + injected + upstream[p..) | да |
| `MapGenerator` | `f(upstream[i])` | да |
| `WhereGenerator` | Фильтрация предикатом | нет |
| `ZipGenerator` | `combine(a[i], b[i])` | да |
| `ConcatGenerator` | left + right | да |

### Ординальная арифметика

```cpp
Ordinal::finite(5)                         // 5
Ordinal::infinity()                        // ω
Ordinal::omega_times(3)                    // ω * 3
Ordinal::infinity() + Ordinal::finite(2)   // ω + 2

// Некоммутативность:
Ordinal::finite(1) + Ordinal::infinity()   // = ω     (1 + ω = ω)
Ordinal::infinity() + Ordinal::finite(1)   // = ω + 1 (ω + 1 ≠ 1 + ω)
```

### Пример: трансфинитный доступ

```cpp
auto a = make_inf(0);       // 0, 1, 2, ...
auto b = make_inf(100);     // 100, 101, ...
auto chain = a->concat(b);  // длина ω * 2

chain->get(5);              // 5   
chain->get(Ordinal(1, 5));  // 105, второй ω-блок
```

## Тесты

| Suite | Что покрывает |
|---|---|
| `lazy_tests` | SlidingCache, базовые LazySequence, map/where/zip/concat |
| `ordinal_tests` | Insert/Concat трансфинитные, ординальный доступ, вложенные вставки |
| `stream_tests` | Все стримы (Sequence, String, File, Lazy) |
| `memory_tape_tests` | MemoryTape: alloc/free, стратегии, фрагментация |
