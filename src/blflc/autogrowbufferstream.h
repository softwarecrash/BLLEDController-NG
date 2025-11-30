//Code from Xtouchautogrowstream
#ifndef AutoGrowBufferStream_h
#define AutoGrowBufferStream_h

#include <Arduino.h>
#include <Stream.h>

#define BUFFER_INCREMENTS 128
#define MAX_BUFFER_SIZE 65536

class AutoGrowBufferStream : public Stream
{
private:
    uint16_t _len;
    uint16_t buffer_size;
    char* _buffer;

public:
    AutoGrowBufferStream();
    ~AutoGrowBufferStream();

    virtual size_t write(uint8_t byte);
    virtual int read();
    virtual int available();
    virtual void flush();
    int peek();

    const uint16_t current_length() const { return _len; }
    const char* get_buffer() const { return _buffer; }
    const char* get_string() const;

    using Print::write;
};

#endif
