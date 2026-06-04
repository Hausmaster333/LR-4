# LR-4

В проекте представлены четыре модуля с интерактивной визуализацией на Dear ImGui:
- Ленивые последовательности с трансфинитной арифметикой
- Симулятор ленты памяти
- Функциональный Stream API
- Архиватор формата `.Z`.

### Lazy Sequence

> <img width="1357" height="821" alt="image" src="https://github.com/user-attachments/assets/64c3063c-e270-4507-b4e3-8e424cbf1390" />

Ленивые последовательности `LazySequence<T>`. Поддерживают **бесконечные** потоки
(натуральные числа, Фибоначчи, степени двойки) через генераторы — элементы вычисляются
по нужде и отправляются в сдвигающийся кэщ.

Ключевая особенность — трансфинитная (ординальная) длина и индексация вида `w * k + n`.
Например, `append` к бесконечной последовательности кладёт элемент на индекс `w` (за
бесконечностью), и достать его можно ординальным геттером:

```cpp
lazy_sequence->get(5);              // обычный финитный индекс
lazy_sequence->get(Ordinal(1, 5));  // индекс w + 5 (второй w-блок)
```

Кнопки: `Get`, `Take`, `Append`, `Prepend`, `Concat`, `Map`, `Where`, `Zip`, `Materialize`.
Визуализатор показывает длину в ординальном формате (`w`, `w * 2`, `w + 3`), состояние кэша и последние материализованные значения.

### Memory Tape

> <img width="1423" height="910" alt="image" src="https://github.com/user-attachments/assets/974d61bc-7850-4cb0-bf58-2f3fd237070f" />

Симулятор ленты памяти фиксированной длины. Поддерживает четыре стратегии аллокации:

- **First-fit** — первый подходящий свободный блок
- **Best-fit** — минимальный подходящий блок
- **Worst-fit** — максимальный подходящий блок
- **Next-fit** — First-fit, но начинает не с начала, а с места выдачи

Поток событий (`Alloc`/`Free`) генерируется бесконечным `AllocEventGenerator` на основе LCG (линейный конгруэнтный генератор).

Имеются ручные команды Alloc/Free, операции фрагментации и уплотнения данных

### Stream API

> <img width="898" height="599" alt="image" src="https://github.com/user-attachments/assets/ca56ddd5-026d-4d60-9b4a-2f533260df9d" />

Пайплайн `StreamAPI<T>` в стиле Java.

```cpp
StreamAPI<std::string>::of(&words)
    .filter(str_ops::starts_with("A"))
    .map<std::string>(str_ops::to_upper())
    .sorted()
    .take(3)
    .to_array();
```

Операции: `filter`, `map<U>`, `sorted`, `take`, `skip` + `to_array`,
`reduce`, `for_each`, `count`. Также строковые операции `str_ops`: `to_upper`, `trim`, `starts_with`, `replace_all` и т.д. В панели пайплайн
собирается из комбинации операций (filter / map / sort / take) и все этапы отображаются в меню.

### LZW архиватор

> <img width="834" height="453" alt="image" src="https://github.com/user-attachments/assets/515bd23c-8dc0-404b-9e66-b163c62cdbf2" />

Архиватор Unix-формата `.Z` (LZW / LZC). Сжатые файлы открываются
сторонними утилитами — 7-Zip (22.00+), `gzip -d`, `uncompress`.

В панели два режима:
- Ввод текста -> сжатие -> расжатие и показ данных
- Работа с файлами (принимает только .z формат для расжатия и txt для сжатия)

## Сборка и запуск

Зависимости: `googletest` (`git clone https://github.com/google/googletest.git`),
Dear ImGui (лежит в `vendor/`) и GLFW.

```console
mingw32-make gui      # GUI (Windows; на Linux — make gui)
./gui
```

Тесты:

```console
mingw32-make lazy_tests ordinal_tests stream_tests memory_tape_tests lzw_tests
./lazy_tests && ./ordinal_tests && ./stream_tests && ./memory_tape_tests && ./lzw_tests
```

Графика: Dear ImGui (immediate-mode GUI) + GLFW (окно/события) + OpenGL.
