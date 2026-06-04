#ifndef LZW_FILE_H
#define LZW_FILE_H

#include "streams/binary_file_read_stream.h"
#include "streams/binary_file_write_stream.h"
#include "compression/lzw_output_stream.h"
#include "compression/lzw_input_stream.h"
#include <cstdint>
#include <string>

struct LzwFileStats {
    size_t source_bytes;     // размер несжатых данных в байтах
    size_t compressed_bytes; // Размер .Z файла в байтах
};

// Сжимает файл в .Z
inline LzwFileStats lzw_compress_file(const std::string& in_path, const std::string& out_path) {
    BinaryFileReadStream<uint8_t> source(in_path);
    BinaryFileWriteStream<uint8_t> sink(out_path);
    LzwOutputStream compressor(&sink);

    source.open();
    compressor.open();

    size_t bytes = 0;
    while (!source.is_end_of_stream()) {
        compressor.write(source.read());
        bytes++;
    }

    compressor.close();
    size_t compressed = sink.get_position();
    source.close();

    return {bytes, compressed};
}

// Разжимает .Z файл
inline LzwFileStats lzw_decompress_file(const std::string& in_path, const std::string& out_path) {
    BinaryFileReadStream<uint8_t> source(in_path);
    BinaryFileWriteStream<uint8_t> sink(out_path);
    LzwInputStream decompressor(&source);

    sink.open();
    decompressor.open();

    size_t bytes = 0;
    while (!decompressor.is_end_of_stream()) {
        sink.write(decompressor.read());
        bytes++;
    }

    size_t compressed = source.get_position();
    decompressor.close();
    sink.close();

    return {bytes, compressed};
}

#endif
