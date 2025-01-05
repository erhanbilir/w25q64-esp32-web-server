// W25Q.h
#ifndef _W25Q_H
#define _W25Q_H

#include <Arduino.h>
#include <SPI.h>

class W25Q {
public:
    W25Q(uint8_t csPin);

    void begin();
    uint32_t getJEDECID();
    void writeData(uint32_t address, const uint8_t *data, uint32_t length);
    void readData(uint32_t address, uint8_t *buffer, size_t length);
    void eraseSector(uint32_t address);
    void eraseBlock(uint32_t address);
    void chipErase();
    bool verifyData(uint32_t address, const uint8_t* data, size_t length);
    bool isBlank(uint32_t address);
    void testAddressing();
    void disableWriteProtection();
    void testChipErase();
    void testSectorErase();
    void testVerify();

private:
    uint8_t _csPin;
    
    void sendCommand(uint8_t cmd, const uint8_t *data, size_t length);
    void readCommand(uint8_t cmd, uint8_t *buffer, size_t length);
    void writeEnable();
    void writeDisable();
    void waitForReady();
};

#endif /* _W25Q_H */