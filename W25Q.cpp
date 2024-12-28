// W25Q.cpp
#include "W25Q.h"

#define CMD_READ_DATA     0x03
#define CMD_PAGE_PROGRAM  0x02
#define CMD_SECTOR_ERASE  0x20
#define CMD_BLOCK_ERASE   0xD8
#define CMD_CHIP_ERASE    0xC7
#define CMD_READ_STATUS   0x05
#define CMD_WRITE_ENABLE  0x06
#define CMD_JEDEC_ID      0x9F

W25Q::W25Q(uint8_t csPin) : _csPin(csPin) {}

void W25Q::begin() {
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);
    SPI.begin();
}

uint32_t W25Q::getJEDECID() {
    uint8_t buffer[3];
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_JEDEC_ID);
    buffer[0] = SPI.transfer(0);
    buffer[1] = SPI.transfer(0);
    buffer[2] = SPI.transfer(0);
    digitalWrite(_csPin, HIGH);
    return (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
}

void W25Q::writeData(uint32_t address, const uint8_t *data, uint32_t length) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    writeEnable();
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_PAGE_PROGRAM); // Page Program command
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    for (size_t i = 0; i < length; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(_csPin, HIGH);
    waitForReady();
    SPI.endTransaction();
}

void W25Q::readData(uint32_t address, uint8_t *buffer, size_t length) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_READ_DATA); // Read command
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    for (size_t i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0);
    }
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
}

void W25Q::eraseSector(uint32_t address) {
    writeEnable();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_SECTOR_ERASE); // Sector Erase command
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
    waitForReady();
}

void W25Q::eraseBlock(uint32_t address) {
    writeEnable();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_BLOCK_ERASE); // Block Erase command
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
    waitForReady();
}

void W25Q::chipErase() {
    writeEnable();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_CHIP_ERASE);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
    waitForReady();
}

bool W25Q::verifyData(uint32_t address, const uint8_t* data, size_t length) {
    uint8_t buffer[256];
    size_t contentLength = sizeof(data);
    for (size_t i = 0; i < contentLength; i += 256) {
        size_t chunkSize = min(contentLength - i, (size_t)256);
        readData(address + i, buffer, chunkSize);
        for (size_t j = 0; j < chunkSize; j++) {
          if (buffer[j] != data[i + j]) {
            Serial.println("Verify başarısız: Veriler eşleşmiyor.");
            return false;
          }
        }
    }
    Serial.println("Verify başarılı: Veriler eşleşiyor.");
    return true;
}

bool W25Q::isBlank(uint32_t address) {
    uint8_t buffer[4096];
    readData(address, buffer, sizeof(buffer));
    for (size_t i = 0; i < 4096; i++) {
        if (buffer[i] != 0xFF) {
            return false;
        }
    }
    return true;
}

void W25Q::sendCommand(uint8_t cmd, const uint8_t *data, size_t length) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(cmd);
    for (size_t i = 0; i < length; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(_csPin, HIGH);
}

void W25Q::readCommand(uint8_t cmd, uint8_t *buffer, size_t length) {
    digitalWrite(_csPin, LOW);
    SPI.transfer(cmd);
    for (size_t i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0);
    }
    digitalWrite(_csPin, HIGH);
}

void W25Q::writeEnable() {
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_WRITE_ENABLE); // Write Enable command
    digitalWrite(_csPin, HIGH);
}

void W25Q::waitForReady() {
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_READ_STATUS);
    while (SPI.transfer(0) & 0x01); // Wait for BUSY bit to clear
    digitalWrite(_csPin, HIGH);
}