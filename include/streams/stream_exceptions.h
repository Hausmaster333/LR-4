#ifndef STREAM_EXCEPTIONS_H
#define STREAM_EXCEPTIONS_H

#include <stdexcept>
#include <string>

struct EndOfStream : public std::runtime_error {
    EndOfStream() : std::runtime_error("End of stream reached") {}
};

struct GoBackUnsupported : public std::logic_error {
    GoBackUnsupported() : std::logic_error("Stream does not support going back") {}
};

struct StreamNotOpen : public std::logic_error {
    StreamNotOpen() : std::logic_error("Stream is not open") {}
};

struct StreamWriteError : public std::runtime_error {
    StreamWriteError(const std::string& message) : std::runtime_error(message) {}
};

#endif
