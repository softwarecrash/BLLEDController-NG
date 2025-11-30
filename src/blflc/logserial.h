#ifndef LOGSERIAL_H
#define LOGSERIAL_H

#include <Arduino.h>
#include <MycilaWebSerial.h>

class AsyncWebServer;

class LogSerialClass : public Stream {
private:
    WebSerial webSerial;
public:
    void begin(unsigned long baud = 115200);
    void begin(AsyncWebServer* server, unsigned long baud = 115200, size_t bufferSize = 100);
    void onMessage(std::function<void(const std::string&)> cb);
    void setBuffer(size_t size);

    int available() override;
    int read() override;
    int peek() override;
    void flush() override;

    size_t write(uint8_t b) override;
    size_t write(const uint8_t* buffer, size_t size) override;

    using Print::write;
    operator bool();
};

extern LogSerialClass LogSerial;

#endif // LOGSERIAL_H
