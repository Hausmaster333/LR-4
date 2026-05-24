#include "streams/stream_exceptions.h"
#include "streams/sequence_read_stream.h"
#include "streams/sequence_write_stream.h"
#include "streams/lazy_read_stream.h"
#include "streams/string_read_stream.h"
#include "streams/file_read_stream.h"
#include "streams/file_write_stream.h"
#include "lazy/lazy_sequence.h"
#include "core/sequence.h"
#include <cstdio>     // для tmpnam/remove
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
