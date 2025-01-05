// W25Q.cpp
#include "W25Q.h"

#define CMD_READ_DATA     0x03
#define CMD_PAGE_PROGRAM  0x02
#define CMD_SECTOR_ERASE  0x20
#define CMD_BLOCK_ERASE   0xD8
#define CMD_CHIP_ERASE    0xC7
#define CMD_READ_STATUS   0x05
#define CMD_WRITE_ENABLE  0x06
#define CMD_WRITE_DISABLE 0x04
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
    eraseSector(address);
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
    writeDisable();
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
    writeDisable();
    delay(450);
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
    writeDisable();
    delay(450);
    waitForReady();
}

void W25Q::chipErase() {
    writeEnable();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_CHIP_ERASE);
    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();
    writeDisable();
    delay(450);
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

void W25Q::testAddressing() {
    uint8_t testData[256];
    uint8_t readBuffer[256];
    for (int i = 0; i < 256; i++) {
        testData[i] = i; 
    }

    uint32_t addresses[] = {0x000000, 0x010000, 0x020000, 0x030000}; 
    for (uint8_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++) {
        uint32_t address = addresses[i];

        Serial.print("Adres: ");
        Serial.println(address, HEX);

        eraseSector(address); 
        writeData(address, testData, sizeof(testData)); 
        readData(address, readBuffer, sizeof(readBuffer)); 

        // Doğrula
        bool isValid = true;
        for (int j = 0; j < 256; j++) {
            if (testData[j] != readBuffer[j]) {
                isValid = false;
                break;
            }
        }

        if (isValid) {
            Serial.println("Veri doğrulandı!");
        } else {
            Serial.println("Doğrulama hatası!");
        }
    }
}

void W25Q::disableWriteProtection() {
    uint8_t status;
    readCommand(0x05, &status, 1);

    Serial.print("Koruma durumu (Status Register): ");
    Serial.println(status, BIN);

    uint8_t disableProtection[2] = {0x00, 0x00}; // BP, TB ve SEC bits
    writeEnable(); //  (CMD_WRITE_ENABLE)
    sendCommand(0x01, disableProtection, 2); // Write Status Register
    waitForReady(); 
    if (status & 0x3C) { // if BP bits are set
        Serial.println("Koruma aktif, devre dışı bırakılıyor...");
        uint8_t disableProtection[2] = {0x01, 0x00}; // reset the Status register
        sendCommand(0x01, disableProtection, 2); // Write Status Register command
        waitForReady();
        Serial.println("Koruma devre dışı bırakıldı.");
    } else {
        Serial.println("Koruma zaten devre dışı.");
    }
}

void W25Q::testChipErase() {
    chipErase();
    Serial.println("Çip tamamen silindi. Kontrol ediliyor...");
    if (isBlank(0x000000)) {
        Serial.println("Çip tamamen boş.");
    } else {
        Serial.println("Çip silinemedi!");
    }
}

void W25Q::testSectorErase() {
    uint32_t address = 0x000000;
    eraseSector(address);
    Serial.print("Sektör silindi. Kontrol ediliyor: ");
    Serial.println(address, HEX);
    if (isBlank(0x000000)) {
        Serial.println("Sektör tamamen boş.");
    } else {
        Serial.println("Sektör silinemedi!");
    }
}

void W25Q::testVerify() {
    uint8_t testData[256];
    for (int i = 0; i < 256; i++) {
        testData[i] = i; 
    }

    uint32_t address = 0x000000;
    writeData(address, testData, sizeof(testData));

    bool isVerified = verifyData(address, testData, sizeof(testData));
    if (isVerified) {
        Serial.println("Doğrulama başarılı!");
    } else {
        Serial.println("Doğrulama başarısız!");
    }
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
    delay(10);
}

void W25Q::writeDisable() {
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_WRITE_ENABLE); // Write Enable command
    digitalWrite(_csPin, HIGH);
    delay(10);
}

void W25Q::waitForReady() {
    delay(10);
    digitalWrite(_csPin, LOW);
    SPI.transfer(CMD_READ_STATUS);
    while (SPI.transfer(0) & 0x01); // Wait for BUSY bit to clear
    digitalWrite(_csPin, HIGH);
}