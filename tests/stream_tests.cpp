#include "streams/stream_exceptions.h"
#include "streams/sequence_read_stream.h"
#include "streams/sequence_write_stream.h"
#include "streams/lazy_read_stream.h"
#include "streams/string_read_stream.h"
#include "streams/file_read_stream.h"
#include "streams/file_write_stream.h"
#include "streams/sequence_stream.h"
#include "streams/string_operations.h"
#include "lazy/lazy_sequence.h"
#include "core/sequence.h"
#include <cstdio>
#include <string>
#include <gtest/gtest.h>

TEST(StreamExceptionsTest, TypesDistinct) {
    EXPECT_NO_THROW({
        try { throw EndOfStream(); }
        catch (const std::runtime_error&) {}
    });
    EXPECT_NO_THROW({
        try { throw GoBackUnsupported(); }
        catch (const std::logic_error&) {}
    });
    EXPECT_NO_THROW({
        try { throw StreamNotOpen(); }
        catch (const std::logic_error&) {}
    });
    EXPECT_NO_THROW({
        try { throw StreamWriteError("test"); }
        catch (const std::runtime_error&) {}
    });
}

// =================== SequenceReadStream ===================

TEST(SequenceReadStreamTest, NullSourceThrows) {
    EXPECT_THROW(SequenceReadStream<int>(nullptr), std::invalid_argument);
}

TEST(SequenceReadStreamTest, ReadBeforeOpenThrows) {
    int items[] = {1, 2, 3};
    MutableArraySequence<int> src(items, 3);
    SequenceReadStream<int> s(&src);

    EXPECT_THROW(s.read(), StreamNotOpen);
}

TEST(SequenceReadStreamTest, ReadAllElements) {
    int items[] = {10, 20, 30};
    MutableArraySequence<int> src(items, 3);
    SequenceReadStream<int> s(&src);
    s.open();

    EXPECT_FALSE(s.is_end_of_stream());
    EXPECT_EQ(s.read(), 10);
    EXPECT_EQ(s.get_position(), 1u);
    EXPECT_EQ(s.read(), 20);
    EXPECT_EQ(s.read(), 30);
    EXPECT_TRUE(s.is_end_of_stream());
    EXPECT_THROW(s.read(), EndOfStream);
}

TEST(SequenceReadStreamTest, SeekForwardThenRead) {
    int items[] = {1, 2, 3, 4, 5};
    MutableArraySequence<int> src(items, 5);
    SequenceReadStream<int> s(&src);
    s.open();

    EXPECT_EQ(s.seek(2), 2u);
    EXPECT_EQ(s.read(), 3);
    EXPECT_EQ(s.read(), 4);
    EXPECT_EQ(s.read(), 5);
    EXPECT_TRUE(s.is_end_of_stream());
}

TEST(SequenceReadStreamTest, SeekBackwardThenRead) {
    int items[] = {1, 2, 3};
    MutableArraySequence<int> src(items, 3);
    SequenceReadStream<int> s(&src);
    s.open();

    s.read();
    s.read();   // позиция = 2
    EXPECT_EQ(s.seek(0), 0u);
    EXPECT_EQ(s.read(), 1);
}

TEST(SequenceReadStreamTest, SeekPastEndClampsToTotal) {
    int items[] = {1, 2};
    MutableArraySequence<int> src(items, 2);
    SequenceReadStream<int> s(&src);
    s.open();

    EXPECT_EQ(s.seek(100), 2u);
    EXPECT_TRUE(s.is_end_of_stream());
}

TEST(SequenceReadStreamTest, IsCanSeekAndGoBackTrue) {
    int items[] = {1};
    MutableArraySequence<int> src(items, 1);
    SequenceReadStream<int> s(&src);

    EXPECT_TRUE(s.is_can_seek());
    EXPECT_TRUE(s.is_can_go_back());
}

TEST(SequenceReadStreamTest, CloseResetsState) {
    int items[] = {1, 2};
    MutableArraySequence<int> src(items, 2);
    SequenceReadStream<int> s(&src);
    s.open();
    s.read();

    s.close();
    EXPECT_FALSE(s.opened());
    EXPECT_EQ(s.get_position(), 0u);
    EXPECT_THROW(s.read(), StreamNotOpen);

    s.open();
    EXPECT_EQ(s.read(), 1);
}

// =================== SequenceWriteStream ===================

TEST(SequenceWriteStreamTest, WriteAppends) {
    MutableArraySequence<int> dst;
    SequenceWriteStream<int> s(&dst);
    s.open();

    EXPECT_EQ(s.write(10), 1u);
    EXPECT_EQ(s.write(20), 2u);
    EXPECT_EQ(s.write(30), 3u);

    EXPECT_EQ(dst.get_count(), 3);
    EXPECT_EQ(dst.get_first(), 10);
    EXPECT_EQ(dst.get_last(), 30);
}

TEST(SequenceWriteStreamTest, GetPositionMatchesCount) {
    MutableArraySequence<int> dst;
    SequenceWriteStream<int> s(&dst);
    s.open();
    s.write(1);
    s.write(2);
    s.write(3);

    EXPECT_EQ(s.get_position(), 3u);
    EXPECT_EQ(s.get_position(), static_cast<size_t>(dst.get_count()));
}

TEST(SequenceWriteStreamTest, WriteBeforeOpenThrows) {
    MutableArraySequence<int> dst;
    SequenceWriteStream<int> s(&dst);

    EXPECT_THROW(s.write(42), StreamNotOpen);
}

TEST(SequenceWriteStreamTest, NullDstThrows) {
    EXPECT_THROW(SequenceWriteStream<int>(nullptr), std::invalid_argument);
}

// =================== LazyReadStream ===================

TEST(LazyReadStreamTest, NullSourceThrows) {
    EXPECT_THROW(LazyReadStream<int>(nullptr), std::invalid_argument);
}

TEST(LazyReadStreamTest, ReadBeforeOpenThrows) {
    int items[] = {1, 2, 3};
    LazySequence<int> lazy(items, 3);
    LazyReadStream<int> s(&lazy);

    EXPECT_THROW(s.read(), StreamNotOpen);
}

TEST(LazyReadStreamTest, EndOfStreamOnFinite) {
    int items[] = {10, 20, 30};
    LazySequence<int> lazy(items, 3);
    LazyReadStream<int> s(&lazy);
    s.open();

    EXPECT_EQ(s.read(), 10);
    EXPECT_EQ(s.read(), 20);
    EXPECT_EQ(s.read(), 30);
    EXPECT_TRUE(s.is_end_of_stream());
    EXPECT_THROW(s.read(), EndOfStream);
}

TEST(LazyReadStreamTest, InfiniteReadFibonacci) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    LazyReadStream<int> s(&fib);
    s.open();

    EXPECT_FALSE(s.is_end_of_stream());
    EXPECT_EQ(s.read(), 0);
    EXPECT_EQ(s.read(), 1);
    EXPECT_EQ(s.read(), 1);
    EXPECT_EQ(s.read(), 2);
    EXPECT_EQ(s.read(), 3);
    EXPECT_EQ(s.read(), 5);
    EXPECT_FALSE(s.is_end_of_stream());
}

TEST(LazyReadStreamTest, SeekForwardOnInfinite) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    LazyReadStream<int> s(&fib);
    s.open();

    EXPECT_EQ(s.seek(5), 5u);
    EXPECT_EQ(s.read(), 5);   // fib(5) = 5
}

TEST(LazyReadStreamTest, GoBackThrows) {
    int items[] = {1, 2, 3};
    LazySequence<int> lazy(items, 3);
    LazyReadStream<int> s(&lazy);
    s.open();

    s.read();
    s.read();   // position = 2
    EXPECT_THROW(s.seek(0), GoBackUnsupported);
    EXPECT_THROW(s.seek(1), GoBackUnsupported);
}

TEST(LazyReadStreamTest, IsCanGoBackFalse) {
    int items[] = {1};
    LazySequence<int> lazy(items, 1);
    LazyReadStream<int> s(&lazy);

    EXPECT_TRUE(s.is_can_seek());
    EXPECT_FALSE(s.is_can_go_back());
}

// =================== StringReadStream ===================

TEST(StringReadStreamTest, ReadCsvLines) {
    auto deser = [](const std::string& s) { return std::stoi(s); };
    StringReadStream<int> s("10\n20\n30", deser);
    s.open();

    EXPECT_EQ(s.read(), 10);
    EXPECT_EQ(s.read(), 20);
    EXPECT_EQ(s.read(), 30);
    EXPECT_TRUE(s.is_end_of_stream());
    EXPECT_THROW(s.read(), EndOfStream);
}

TEST(StringReadStreamTest, EmptyStringIsEOS) {
    auto deser = [](const std::string& s) { return std::stoi(s); };
    StringReadStream<int> s("", deser);
    s.open();

    EXPECT_TRUE(s.is_end_of_stream());
    EXPECT_THROW(s.read(), EndOfStream);
}

TEST(StringReadStreamTest, SeekBackAndForth) {
    auto deser = [](const std::string& s) { return std::stoi(s); };
    StringReadStream<int> s("1\n2\n3\n4\n5", deser);
    s.open();

    EXPECT_EQ(s.read(), 1);
    EXPECT_EQ(s.read(), 2);
    EXPECT_EQ(s.read(), 3);
    EXPECT_EQ(s.seek(1), 1u);
    EXPECT_EQ(s.read(), 2);
    EXPECT_EQ(s.read(), 3);
    EXPECT_EQ(s.read(), 4);
    EXPECT_EQ(s.read(), 5);
}

TEST(StringReadStreamTest, SeekForwardSkips) {
    auto deser = [](const std::string& s) { return std::stoi(s); };
    StringReadStream<int> s("1\n2\n3\n4\n5", deser);
    s.open();

    EXPECT_EQ(s.seek(3), 3u);
    EXPECT_EQ(s.read(), 4);
}

// =================== File streams ===================

// Хелпер: получить уникальный временный путь в текущей директории.
static std::string make_tmp_path(const std::string& tag) {
    return std::string("__test_") + tag + ".tmp";
}

TEST(FileWriteStreamTest, WriteSerializesEachOnNewLine) {
    std::string path = make_tmp_path("fw1");
    auto serializer = [](const int& v) { return std::to_string(v); };

    {
        FileWriteStream<int> w(path, serializer);
        w.open();
        EXPECT_EQ(w.write(10), 1u);
        EXPECT_EQ(w.write(20), 2u);
        EXPECT_EQ(w.write(30), 3u);
        w.close();
    }

    // Прочитаем обратно вручную через ifstream
    std::ifstream in(path);
    std::string line;
    int expected[] = {10, 20, 30};
    int idx = 0;
    while (std::getline(in, line)) {
        EXPECT_EQ(std::stoi(line), expected[idx]);
        idx++;
    }
    EXPECT_EQ(idx, 3);
    in.close();
    std::remove(path.c_str());
}

TEST(FileWriteStreamTest, WriteBeforeOpenThrows) {
    auto serializer = [](const int& v) { return std::to_string(v); };
    FileWriteStream<int> w(make_tmp_path("fw2"), serializer);

    EXPECT_THROW(w.write(1), StreamNotOpen);
}

TEST(FileReadStreamTest, WriteThenReadRoundtrip) {
    std::string path = make_tmp_path("frw");
    auto serializer = [](const int& v) { return std::to_string(v); };
    auto deserializer = [](const std::string& s) { return std::stoi(s); };

    {
        FileWriteStream<int> w(path, serializer);
        w.open();

        for (int idx = 1; idx <= 5; ++idx) {
            w.write(idx * 10);
        }

        w.close();
    }

    FileReadStream<int> r(path, deserializer);
    r.open();
    EXPECT_EQ(r.read(), 10);
    EXPECT_EQ(r.read(), 20);
    EXPECT_EQ(r.read(), 30);
    EXPECT_EQ(r.read(), 40);
    EXPECT_EQ(r.read(), 50);
    EXPECT_TRUE(r.is_end_of_stream());
    EXPECT_THROW(r.read(), EndOfStream);
    r.close();
    std::remove(path.c_str());
}

TEST(FileReadStreamTest, OpenMissingFileThrows) {
    auto deser = [](const std::string& s) { return std::stoi(s); };
    FileReadStream<int> r("__definitely_not_exists__.tmp", deser);
    EXPECT_THROW(r.open(), StreamWriteError);
}

TEST(FileReadStreamTest, SeekForwardSkipsLines) {
    std::string path = make_tmp_path("seek_fwd");
    auto serializer = [](const int& v) { return std::to_string(v); };
    auto deserializer = [](const std::string& s) { return std::stoi(s); };

    {
        FileWriteStream<int> w(path, serializer);
        w.open();

        for (int idx = 1; idx <= 5; ++idx) {
            w.write(idx);
        }

        w.close();
    }

    FileReadStream<int> r(path, deserializer);
    r.open();
    EXPECT_EQ(r.seek(2), 2u);
    EXPECT_EQ(r.read(), 3);
    r.close();
    std::remove(path.c_str());
}

TEST(FileReadStreamTest, SeekBackToLine0) {
    std::string path = make_tmp_path("seek_back");
    auto serializer = [](const int& v) { return std::to_string(v); };
    auto deserializer = [](const std::string& s) { return std::stoi(s); };

    {
        FileWriteStream<int> w(path, serializer);
        w.open();

        for (int idx = 1; idx <= 5; ++idx) {
            w.write(idx);
        }

        w.close();
    }

    FileReadStream<int> r(path, deserializer);
    r.open();
    EXPECT_EQ(r.read(), 1);
    EXPECT_EQ(r.read(), 2);
    EXPECT_EQ(r.read(), 3);
    EXPECT_EQ(r.seek(0), 0u);
    EXPECT_EQ(r.read(), 1);
    EXPECT_EQ(r.read(), 2);
    r.close();
    std::remove(path.c_str());
}

// =================== SequenceStream ===================

TEST(SequenceStreamTest, Of_FromArray) {
    int items[] = {10, 20, 30};
    auto* result = SequenceStream<int>::of(items, 3).to_array();

    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), 10);
    EXPECT_EQ(result->get(1), 20);
    EXPECT_EQ(result->get(2), 30);
    delete result;
}

TEST(SequenceStreamTest, Of_FromSequence) {
    int items[] = {1, 2, 3};
    MutableArraySequence<int> src(items, 3);
    auto* result = SequenceStream<int>::of(&src).to_array();

    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(2), 3);
    delete result;
}

TEST(SequenceStreamTest, Filter) {
    int items[] = {1, 2, 3, 4, 5};
    auto* result = SequenceStream<int>::of(items, 5).filter([](const int& x) { return x % 2 == 0; }).to_array();

    EXPECT_EQ(result->get_count(), 2);
    EXPECT_EQ(result->get(0), 2);
    EXPECT_EQ(result->get(1), 4);
    delete result;
}

TEST(SequenceStreamTest, Map) {
    int items[] = {1, 2, 3};
    auto* result = SequenceStream<int>::of(items, 3).map<int>([](const int& x) { return x * 2; }).to_array();

    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), 2);
    EXPECT_EQ(result->get(1), 4);
    EXPECT_EQ(result->get(2), 6);
    delete result;
}

TEST(SequenceStreamTest, MapChangeType) {
    int items[] = {1, 2, 3};
    auto* result = SequenceStream<int>::of(items, 3).map<std::string>([](const int& x) { return std::to_string(x); }).to_array();

    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), "1");
    EXPECT_EQ(result->get(1), "2");
    EXPECT_EQ(result->get(2), "3");
    delete result;
}

TEST(SequenceStreamTest, Sorted_Default) {
    int items[] = {3, 1, 4, 1, 5};
    auto* result = SequenceStream<int>::of(items, 5).sorted().to_array();

    EXPECT_EQ(result->get_count(), 5);
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 1);
    EXPECT_EQ(result->get(2), 3);
    EXPECT_EQ(result->get(3), 4);
    EXPECT_EQ(result->get(4), 5);
    delete result;
}

TEST(SequenceStreamTest, Sorted_Custom) {
    int items[] = {3, 1, 4, 1, 5};
    auto* result = SequenceStream<int>::of(items, 5).sorted([](const int& a, const int& b) { return a > b; }).to_array();

    EXPECT_EQ(result->get_count(), 5);
    EXPECT_EQ(result->get(0), 5);
    EXPECT_EQ(result->get(1), 4);
    EXPECT_EQ(result->get(2), 3);
    EXPECT_EQ(result->get(3), 1);
    EXPECT_EQ(result->get(4), 1);
    delete result;
}

TEST(SequenceStreamTest, Take) {
    int items[] = {10, 20, 30, 40, 50};
    auto* result = SequenceStream<int>::of(items, 5).take(3).to_array();

    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), 10);
    EXPECT_EQ(result->get(2), 30);
    delete result;
}

TEST(SequenceStreamTest, Skip) {
    int items[] = {10, 20, 30, 40, 50};
    auto* result = SequenceStream<int>::of(items, 5).skip(2).to_array();

    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), 30);
    EXPECT_EQ(result->get(1), 40);
    EXPECT_EQ(result->get(2), 50);
    delete result;
}

TEST(SequenceStreamTest, Reduce) {
    int items[] = {1, 2, 3, 4, 5};
    int sum = SequenceStream<int>::of(items, 5).reduce([](const int& acc, const int& x) { return acc + x; }, 0);

    EXPECT_EQ(sum, 15);
}

TEST(SequenceStreamTest, ForEach) {
    int items[] = {10, 20, 30};
    int total = 0;
    SequenceStream<int>::of(items, 3).for_each([&total](const int& x) { total += x; });

    EXPECT_EQ(total, 60);
}

TEST(SequenceStreamTest, Count) {
    int items[] = {1, 2, 3, 4, 5};
    int count = SequenceStream<int>::of(items, 5).filter([](const int& x) { return x > 2; }).count();

    EXPECT_EQ(count, 3);
}

TEST(SequenceStreamTest, FullPipeline) {
    int items[] = {5, 3, 8, 1, 9, 2, 7};
    auto* result = SequenceStream<int>::of(items, 7).filter([](const int& x) { return x > 3; }).sorted().take(3).to_array();

    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), 5);
    EXPECT_EQ(result->get(1), 7);
    EXPECT_EQ(result->get(2), 8);
    delete result;
}

TEST(SequenceStreamTest, EmptyStream) {
    auto* result = SequenceStream<int>::of(nullptr, 0).to_array();
    EXPECT_EQ(result->get_count(), 0);
    delete result;
}

TEST(SequenceStreamTest, ConsumedStreamThrows) {
    int items[] = {1, 2, 3};
    auto stream = SequenceStream<int>::of(items, 3);
    auto* first = stream.to_array();
    EXPECT_THROW(stream.to_array(), std::logic_error);
    delete first;
}

// =================== String Operations

TEST(StringOpsTest, ToUpper) {
    std::string items[] = {"hello", "World"};
    auto* result = SequenceStream<std::string>::of(items, 2).map<std::string>(str_ops::to_upper()).to_array();

    EXPECT_EQ(result->get(0), "HELLO");
    EXPECT_EQ(result->get(1), "WORLD");
    delete result;
}

TEST(StringOpsTest, ToLower) {
    std::string items[] = {"HELLO", "World"};
    auto* result = SequenceStream<std::string>::of(items, 2).map<std::string>(str_ops::to_lower()).to_array();

    EXPECT_EQ(result->get(0), "hello");
    EXPECT_EQ(result->get(1), "world");
    delete result;
}

TEST(StringOpsTest, Trim) {
    std::string items[] = {"  hi  ", "no_spaces", " left", "right "};
    auto* result = SequenceStream<std::string>::of(items, 4).map<std::string>(str_ops::trim()).to_array();

    EXPECT_EQ(result->get(0), "hi");
    EXPECT_EQ(result->get(1), "no_spaces");
    EXPECT_EQ(result->get(2), "left");
    EXPECT_EQ(result->get(3), "right");
    delete result;
}

TEST(StringOpsTest, Substr) {
    std::string items[] = {"abcdef", "xyz"};
    auto* result = SequenceStream<std::string>::of(items, 2).map<std::string>(str_ops::substr(2, 3)).to_array();

    EXPECT_EQ(result->get(0), "cde");
    EXPECT_EQ(result->get(1), "z");
    delete result;
}

TEST(StringOpsTest, ReplaceAll) {
    std::string items[] = {"aXbXc", "noX"};
    auto* result = SequenceStream<std::string>::of(items, 2).map<std::string>(str_ops::replace_all("X", "-")).to_array();

    EXPECT_EQ(result->get(0), "a-b-c");
    EXPECT_EQ(result->get(1), "no-");
    delete result;
}

TEST(StringOpsTest, PrependAppend) {
    std::string items[] = {"world"};
    auto* result = SequenceStream<std::string>::of(items, 1).map<std::string>(str_ops::prepend("hello ")).map<std::string>(str_ops::append("!")).to_array();

    EXPECT_EQ(result->get(0), "hello world!");
    delete result;
}

TEST(StringOpsTest, StartsWith) {
    std::string items[] = {"apple", "banana", "avocado", "cherry"};
    auto* result = SequenceStream<std::string>::of(items, 4).filter(str_ops::starts_with("a")).to_array();

    EXPECT_EQ(result->get_count(), 2);
    EXPECT_EQ(result->get(0), "apple");
    EXPECT_EQ(result->get(1), "avocado");
    delete result;
}

TEST(StringOpsTest, EndsWith) {
    std::string items[] = {"test.cpp", "main.h", "data.cpp", "readme.md"};
    auto* result = SequenceStream<std::string>::of(items, 4).filter(str_ops::ends_with(".cpp")).to_array();

    EXPECT_EQ(result->get_count(), 2);
    EXPECT_EQ(result->get(0), "test.cpp");
    EXPECT_EQ(result->get(1), "data.cpp");
    delete result;
}

TEST(StringOpsTest, Contains) {
    std::string items[] = {"hello world", "foo", "world cup", "bar"};
    auto* result = SequenceStream<std::string>::of(items, 4).filter(str_ops::contains("world")).to_array();

    EXPECT_EQ(result->get_count(), 2);
    EXPECT_EQ(result->get(0), "hello world");
    EXPECT_EQ(result->get(1), "world cup");
    delete result;
}

TEST(StringOpsTest, MinLength) {
    std::string items[] = {"a", "bb", "ccc", "dddd"};
    auto* result = SequenceStream<std::string>::of(items, 4).filter(str_ops::min_length(3)).to_array();

    EXPECT_EQ(result->get_count(), 2);
    EXPECT_EQ(result->get(0), "ccc");
    EXPECT_EQ(result->get(1), "dddd");
    delete result;
}

TEST(StringOpsTest, IsNotEmpty) {
    std::string items[] = {"hello", "", "world", ""};
    auto* result = SequenceStream<std::string>::of(items, 4).filter(str_ops::is_not_empty()).to_array();

    EXPECT_EQ(result->get_count(), 2);
    EXPECT_EQ(result->get(0), "hello");
    EXPECT_EQ(result->get(1), "world");
    delete result;
}

TEST(StringOpsTest, Join) {
    std::string items[] = {"a", "b", "c"};
    auto* arr = SequenceStream<std::string>::of(items, 3).to_array();
    std::string joined = str_ops::join(arr, ", ");

    EXPECT_EQ(joined, "a, b, c");
    delete arr;
}

TEST(StringOpsTest, FullPipeline) {
    std::string items[] = {"  Apple  ", "  banana  ", "Avocado", "cherry", "  apricot  "};
    auto* result = SequenceStream<std::string>::of(items, 5)
        .map<std::string>(str_ops::trim())
        .filter(str_ops::starts_with("A"))
        .map<std::string>(str_ops::to_upper())
        .sorted()
        .to_array();

    EXPECT_EQ(result->get_count(), 2);
    EXPECT_EQ(result->get(0), "APPLE");
    EXPECT_EQ(result->get(1), "AVOCADO");

    std::string joined = str_ops::join(result, " | ");
    EXPECT_EQ(joined, "APPLE | AVOCADO");
    delete result;
}
