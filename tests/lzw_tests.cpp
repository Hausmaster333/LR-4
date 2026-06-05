#include "compression/lzw_output_stream.h"
#include "compression/lzw_input_stream.h"
#include "compression/lzw_file.h"
#include "streams/sequence_read_stream.h"
#include "streams/sequence_write_stream.h"
#include "streams/binary_file_write_stream.h"
#include "streams/binary_file_read_stream.h"
#include "core/sequence.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// Сжимает data в .Z
void compress_mem(const uint8_t* data, int count, MutableArraySequence<uint8_t>& out) {
    SequenceWriteStream<uint8_t> backing(&out);
    LzwOutputStream compressor(&backing);
    compressor.open();
    for (int index = 0; index < count; index++) {
        compressor.write(data[index]);
    }

    compressor.close();
}

// Разжимает .Z обратно
void decompress_mem(const MutableArraySequence<uint8_t>& z, MutableArraySequence<uint8_t>& out) {
    SequenceReadStream<uint8_t> backing(&z);
    LzwInputStream decompressor(&backing);
    decompressor.open();
    while (!decompressor.is_end_of_stream()) {
        out.append(decompressor.read());
    }

    decompressor.close();
}

// Полный цикл
void check_cycle(const uint8_t* data, int count) {
    MutableArraySequence<uint8_t> compressed;
    compress_mem(data, count, compressed);

    MutableArraySequence<uint8_t> restored;
    decompress_mem(compressed, restored);

    ASSERT_EQ(restored.get_count(), count);
    for (int index = 0; index < count; index++) {
        ASSERT_EQ(restored.get(index), data[index]) << "mismatch at index=" << index;
    }
}

void write_bytes_file(const std::string& path, const uint8_t* data, int count) {
    BinaryFileWriteStream<uint8_t> writer(path);
    writer.open();
    for (int index = 0; index < count; index++) {
        writer.write(data[index]);
    }

    writer.close();
}

void read_bytes_file(const std::string& path, MutableArraySequence<uint8_t>& out) {
    BinaryFileReadStream<uint8_t> reader(path);
    reader.open();
    while (!reader.is_end_of_stream()) {
        out.append(reader.read());
    }

    reader.close();
}

TEST(LzwTest, RoundTripEmpty) {
    check_cycle(nullptr, 0);
}

TEST(LzwTest, RoundTripSingleByte) {
    uint8_t data[] = {0x41};
    check_cycle(data, 1);
}

TEST(LzwTest, RoundTripSmall) {
    const char* text = "TOBEORNOTTOBEORTOBEORNOT";
    check_cycle(reinterpret_cast<const uint8_t*>(text), static_cast<int>(std::strlen(text)));
}

TEST(LzwTest, RoundTripRepetitive) {
    MutableArraySequence<uint8_t> data;
    for (int index = 0; index < 4000; index++) data.append(static_cast<uint8_t>('A' + (index % 4)));

    uint8_t* raw = new uint8_t[data.get_count()];
    for (int index = 0; index < data.get_count(); index++) {
        raw[index] = data.get(index);
    }

    check_cycle(raw, data.get_count());

    delete[] raw;
}

TEST(LzwTest, RoundTripLargeRandom) {
    const int count = 70000;
    uint8_t* raw = new uint8_t[count];
    uint32_t state = 2463534242u;
    for (int index = 0; index < count; index++) {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        raw[index] = static_cast<uint8_t>(state & 0xFF);
    }

    check_cycle(raw, count);
    delete[] raw;
}

TEST(LzwTest, RoundTripAllBytes) {
    uint8_t data[256];
    for (int index = 0; index < 256; index++) {
        data[index] = static_cast<uint8_t>(index);
    }

    check_cycle(data, 256);
}

TEST(LzwTest, HeaderBytes) {
    uint8_t data[] = {0x41, 0x42, 0x43};
    MutableArraySequence<uint8_t> compressed;
    compress_mem(data, 3, compressed);

    ASSERT_GE(compressed.get_count(), 3);
    EXPECT_EQ(compressed.get(0), 0x1F);
    EXPECT_EQ(compressed.get(1), 0x9D);
    EXPECT_EQ(compressed.get(2), 0x90);
}

TEST(LzwTest, ExactVectorAAAAA) {
    uint8_t data[] = {0x41, 0x41, 0x41, 0x41, 0x41};
    MutableArraySequence<uint8_t> compressed;
    compress_mem(data, 5, compressed);

    const uint8_t expected[] = {0x1F, 0x9D, 0x90, 0x41, 0x02, 0x06, 0x04};
    ASSERT_EQ(compressed.get_count(), 7);
    for (int index = 0; index < 7; index++) {
        EXPECT_EQ(compressed.get(index), expected[index]) << "at byte " << index;
    }
}

TEST(LzwTest, FileRoundTrip) {
    const char* text = "ABABABABABABABABABABA_and_some_more_repeated_text_text_text!!!";
    int count = static_cast<int>(std::strlen(text));
    std::string in_path = "lzw_in.bin";
    std::string z_path = "archive.Z";
    std::string out_path = "lzw_out.bin";

    write_bytes_file(in_path, reinterpret_cast<const uint8_t*>(text), count);

    LzwFileStats compressed = lzw_compress_file(in_path, z_path);
    EXPECT_EQ(compressed.source_bytes, static_cast<size_t>(count));
    EXPECT_GT(compressed.compressed_bytes, 3u);

    LzwFileStats decompressed = lzw_decompress_file(z_path, out_path);
    EXPECT_EQ(decompressed.source_bytes, static_cast<size_t>(count));

    MutableArraySequence<uint8_t> restored;
    read_bytes_file(out_path, restored);
    ASSERT_EQ(restored.get_count(), count);
    for (int index = 0; index < count; index++) {
        EXPECT_EQ(restored.get(index), static_cast<uint8_t>(text[index])) << "at index=" << index;
    }

    std::remove(in_path.c_str());
    std::remove(z_path.c_str());
    std::remove(out_path.c_str());
}
