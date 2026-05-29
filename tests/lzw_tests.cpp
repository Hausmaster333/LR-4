#include "compression/lzw.h"
#include "compression/lzw_stream.h"
#include "streams/sequence_read_stream.h"
#include "streams/sequence_write_stream.h"
#include "core/sequence.h"
#include <gtest/gtest.h>
#include <cstdint>

MutableArraySequence<uint8_t>* make_bytes(const char* str) {
    auto* seq = new MutableArraySequence<uint8_t>();
    for (int idx = 0; str[idx] != '\0'; idx++) {
        seq->append(static_cast<uint8_t>(str[idx]));
    }
    return seq;
}

bool sequences_equal(const MutableArraySequence<uint8_t>* a, const MutableArraySequence<uint8_t>* b) {
    if (a->get_count() != b->get_count()) return false;

    for (int idx = 0; idx < a->get_count(); idx++) {
        if (a->get(idx) != b->get(idx)) return false;
    }

    return true;
}

TEST(LzwTest, EmptyInput) {
    MutableArraySequence<uint8_t> empty;
    auto* compressed = lzw_compress(&empty);
    EXPECT_EQ(compressed->get_count(), 0);

    auto* decompressed = lzw_decompress(compressed);
    EXPECT_EQ(decompressed->get_count(), 0);

    delete compressed;
    delete decompressed;
}

TEST(LzwTest, SingleByte) {
    auto* input = make_bytes("A");
    auto* compressed = lzw_compress(input);
    EXPECT_EQ(compressed->get_count(), 1);
    EXPECT_EQ(compressed->get(0), static_cast<uint16_t>('A'));

    auto* decompressed = lzw_decompress(compressed);
    EXPECT_TRUE(sequences_equal(input, decompressed));

    delete input;
    delete compressed;
    delete decompressed;
}

TEST(LzwTest, AllSameBytes) {
    auto* input = make_bytes("AAAAAAA");
    auto* compressed = lzw_compress(input);

    EXPECT_LT(compressed->get_count(), input->get_count());

    auto* decompressed = lzw_decompress(compressed);
    EXPECT_TRUE(sequences_equal(input, decompressed));

    delete input;
    delete compressed;
    delete decompressed;
}

TEST(LzwTest, SimplePattern) {
    auto* input = make_bytes("ABABABAB");
    auto* compressed = lzw_compress(input);

    auto* decompressed = lzw_decompress(compressed);
    EXPECT_TRUE(sequences_equal(input, decompressed));

    delete input;
    delete compressed;
    delete decompressed;
}

TEST(LzwTest, RoundTrip_Text) {
    auto* input = make_bytes("The quick brown fox jumps over the lazy dog. " "The quick brown fox jumps over the lazy dog.");
    auto* compressed = lzw_compress(input);
    auto* decompressed = lzw_decompress(compressed);

    EXPECT_TRUE(sequences_equal(input, decompressed));

    delete input;
    delete compressed;
    delete decompressed;
}

TEST(LzwTest, RoundTrip_Binary) {
    auto* input = new MutableArraySequence<uint8_t>();
    for (int i = 0; i < 256; i++) {
        input->append(static_cast<uint8_t>(i));
    }

    auto* compressed = lzw_compress(input);
    auto* decompressed = lzw_decompress(compressed);

    EXPECT_TRUE(sequences_equal(input, decompressed));

    delete input;
    delete compressed;
    delete decompressed;
}

TEST(LzwTest, RoundTrip_Large) {
    auto* input = new MutableArraySequence<uint8_t>();
    uint32_t state = 12345;
    for (int i = 0; i < 10000; i++) {
        state = state * 1103515245 + 12345;
        input->append(static_cast<uint8_t>((state >> 16) & 0xFF));
    }

    auto* compressed = lzw_compress(input);
    auto* decompressed = lzw_decompress(compressed);

    EXPECT_TRUE(sequences_equal(input, decompressed));

    delete input;
    delete compressed;
    delete decompressed;
}

TEST(LzwTest, CompressesWell) {
    auto* input = new MutableArraySequence<uint8_t>();
    for (int i = 0; i < 1000; i++) {
        input->append(static_cast<uint8_t>('A' + (i % 4)));
    }

    auto* compressed = lzw_compress(input);

    EXPECT_LT(compressed->get_count(), static_cast<int>(input->get_count() / 2));

    auto* decompressed = lzw_decompress(compressed);
    EXPECT_TRUE(sequences_equal(input, decompressed));

    delete input;
    delete compressed;
    delete decompressed;
}

// =================== LzwStream ===================

TEST(LzwStreamTest, WriteAndReadBack) {
    MutableArraySequence<uint16_t> code_buffer;
    SequenceWriteStream<uint16_t> backing_out(&code_buffer);
    LzwOutputStream out(&backing_out);

    out.open();
    const char* text = "Hello, LZW compression!";
    for (int i = 0; text[i] != '\0'; i++) {
        out.write(static_cast<uint8_t>(text[i]));
    }
    out.close();

    EXPECT_GT(code_buffer.get_count(), 0);

    SequenceReadStream<uint16_t> backing_in(&code_buffer);
    LzwInputStream in(&backing_in);
    in.open();

    std::string result;
    while (!in.is_end_of_stream()) {
        result += static_cast<char>(in.read());
    }
    in.close();

    EXPECT_EQ(result, "Hello, LZW compression!");
}

TEST(LzwStreamTest, EmptyStream) {
    MutableArraySequence<uint16_t> code_buffer;
    SequenceWriteStream<uint16_t> backing_out(&code_buffer);
    LzwOutputStream out(&backing_out);

    out.open();
    out.close();

    SequenceReadStream<uint16_t> backing_in(&code_buffer);
    LzwInputStream in(&backing_in);
    in.open();

    EXPECT_TRUE(in.is_end_of_stream());
    EXPECT_THROW(in.read(), EndOfStream);
    in.close();
}

TEST(LzwStreamTest, SingleByte) {
    MutableArraySequence<uint16_t> code_buffer;
    SequenceWriteStream<uint16_t> backing_out(&code_buffer);
    LzwOutputStream out(&backing_out);

    out.open();
    out.write(65);
    out.close();

    SequenceReadStream<uint16_t> backing_in(&code_buffer);
    LzwInputStream in(&backing_in);
    in.open();

    EXPECT_FALSE(in.is_end_of_stream());
    EXPECT_EQ(in.read(), 65);
    EXPECT_TRUE(in.is_end_of_stream());
    in.close();
}

TEST(LzwStreamTest, LargeData) {
    MutableArraySequence<uint16_t> code_buffer;
    SequenceWriteStream<uint16_t> backing_out(&code_buffer);
    LzwOutputStream out(&backing_out);

    out.open();
    uint32_t state = 42;
    MutableArraySequence<uint8_t> original;
    for (int i = 0; i < 5000; i++) {
        state = state * 1103515245 + 12345;
        uint8_t byte = static_cast<uint8_t>((state >> 16) & 0xFF);
        out.write(byte);
        original.append(byte);
    }
    out.close();

    SequenceReadStream<uint16_t> backing_in(&code_buffer);
    LzwInputStream in(&backing_in);
    in.open();

    for (int i = 0; i < original.get_count(); i++) {
        ASSERT_FALSE(in.is_end_of_stream()) << "EOS at i=" << i;
        EXPECT_EQ(in.read(), original.get(i)) << "Mismatch at i=" << i;
    }
    EXPECT_TRUE(in.is_end_of_stream());
    in.close();
}

TEST(LzwStreamTest, ReadBeforeOpenThrows) {
    MutableArraySequence<uint16_t> code_buffer;
    SequenceReadStream<uint16_t> backing(&code_buffer);
    LzwInputStream in(&backing);

    EXPECT_THROW(in.read(), StreamNotOpen);
}

TEST(LzwStreamTest, WriteBeforeOpenThrows) {
    MutableArraySequence<uint16_t> code_buffer;
    SequenceWriteStream<uint16_t> backing(&code_buffer);
    LzwOutputStream out(&backing);

    EXPECT_THROW(out.write(42), StreamNotOpen);
}
