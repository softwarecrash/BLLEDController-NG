#include "autogrowbufferstream.h"
#include "logserial.h"

AutoGrowBufferStream::AutoGrowBufferStream() {
    _len = 0;
    _buffer = (char*)malloc(BUFFER_INCREMENTS);
    buffer_size = BUFFER_INCREMENTS;
}

AutoGrowBufferStream::~AutoGrowBufferStream() {
    free(_buffer);
}

size_t AutoGrowBufferStream::write(uint8_t byte) {
    if (_len + 1 > MAX_BUFFER_SIZE) {
        LogSerial.println(F("Max buffer size reached — flushing"));
        flush();
        return 0;
    }
    if (_len + 1 > buffer_size) {
        auto tmp = (char*)realloc(_buffer, buffer_size + BUFFER_INCREMENTS);
        if (tmp == NULL) {
            LogSerial.println(F("Failed to grow buffer"));
            return 0;
        }
        _buffer = tmp;
        buffer_size += BUFFER_INCREMENTS;
    }
    _buffer[_len] = byte;
    _len++;
    return 1;
}

int AutoGrowBufferStream::read() {
    return 0;
}

int AutoGrowBufferStream::available() {
    return 1;
}

void AutoGrowBufferStream::flush() {
    _len = 0;
    _buffer = (char*)realloc(_buffer, BUFFER_INCREMENTS);
    buffer_size = BUFFER_INCREMENTS;
}

int AutoGrowBufferStream::peek() {
    return 0;
}

const char* AutoGrowBufferStream::get_string() const {
    _buffer[_len] = '\0';
    return _buffer;
}
