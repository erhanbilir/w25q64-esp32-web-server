#include <Arduino.h>
#include <SPI.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "W25Q.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

W25Q flashChip(5);
AsyncWebServer server(80);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Flash Programming</title>
    <style>
        /* Genel gövde stili */
        body {
            font-family: 'Arial', sans-serif;
            margin: 0;
            padding: 0;
            background: linear-gradient(135deg, #4a90e2 0%, #2a5d99 100%);
            color: white;
            text-align: center;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
        }

        .container {
            max-width: 1400px;
            margin: auto;
            background: rgba(255, 255, 255, 0.1);
            border: 1px solid rgba(255, 255, 255, 0.3);
            padding: 50px;
            border-radius: 15px;
            box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.2);
        }

        h1 {
            font-size: 2rem;
            margin-bottom: 20px;
            text-shadow: 2px 2px 8px rgba(0, 0, 0, 0.4);
        }

        .button-group button {
            background: linear-gradient(135deg, #ff6b6b 0%, #ff758c 100%);
            color: white;
            border: none;
            padding: 12px 24px;
            margin: 5px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 1rem;
            transition: all 0.3s ease-in-out;
            box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.2);
        }

        .button-group button:hover {
            transform: translateY(-3px);
            box-shadow: 0px 6px 12px rgba(0, 0, 0, 0.3);
        }

        .button-group button:active {
            transform: translateY(0);
            box-shadow: 0px 2px 4px rgba(0, 0, 0, 0.2);
        }

        .button-group button:disabled {
            background: linear-gradient(135deg, #aaa 0%, #888 100%);
            cursor: not-allowed;
        }

        .dropdown {
            margin: 10px 0;
        }

        select {
            padding: 10px;
            font-size: 1rem;
            border-radius: 15px;
            border: none;
            background: rgba(255, 255, 255, 0.6);
            color: black;
            box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.2);
        }

        .output {
            background: rgba(255, 255, 255, 0.1);
            color: black;
            padding: 15px;
            border-radius: 15px;
            margin-top: 20px;
            text-align: left;
            overflow-x: auto;
            overflow-y: auto;
            width: 100%;
            max-height: 300px;
            box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.2);
        }

        .hex-view {
            display: grid;
            grid-template-columns: 80px repeat(16, 40px) 1fr;
            gap: 10px;
            align-items: center;
            font-family: monospace;
            font-size: 14px;
            color: #fff;
        }

        .hex-view div {
            background: rgba(255, 255, 255, 0.6);
            color: #000000;
            padding: 5px;
            border-radius: 5px;
            text-align: center;
            box-shadow: 0px 2px 4px rgba(0, 0, 0, 0.2);
        }

        .info {
            background: rgba(255, 255, 255, 0.6);
            color: black;
            padding: 15px;
            margin-top: 20px;
            border-radius: 15px;
            box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.2);
            font-size: 1rem;
        }

        .info:hover {
            background: rgba(255, 255, 255, 0.2);
            box-shadow: 0px 6px 12px rgba(0, 0, 0, 0.3);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Flash Programming</h1>

        <div class="button-group">
            <input type="file" id="selectFile" style="margin-bottom: 10px;">
            <button id="upload" >Upload</button>
            <button id="write" onclick="handleAction('write')" >Write</button>
            <button id="read" onclick="handleAction('read')" >Read</button>
            <button id="deletesector" onclick="handleAction('deletesector')" >Delete Sector</button>
            <button id="deleteblock" onclick="handleAction('deleteblock')" >Delete Block</button>
            <button id="blank" onclick="handleAction('blank')" >Blank</button>
            <button id="verify" onclick="handleAction('verify')" >Verify</button>
            <button id="jedec" onclick="handleAction('jedec')">Get JEDEC ID</button>
            <div class="dropdown">
                <label for="sector">Sector (4KB): </label>
                <select id="sector">
                    <option value="">Select Sector</option>
                    <script>
                        const sectorDropdown = document.getElementById('sector');

                        for (let i = 0; i <= 15; i++) {
                            const option = document.createElement('option');
                            option.value = i;
                            option.textContent = `Sector ${i}`;
                            sectorDropdown.appendChild(option);
                        }
                    </script>
                </select>
            </div>
    
            <div class="dropdown">
                <label for="block">Block (64KB): </label>
                <select id="block">
                    <option value="">Select Block</option>
                    <script>
                        const blockDropdown = document.getElementById('block');

                        for (let i = 0; i <= 127; i++) {
                            const option = document.createElement('option');
                            option.value = i;
                            option.textContent = `Block ${i}`;
                            blockDropdown.appendChild(option);
                        }
                    </script>
                </select>
            </div>
        </div>



        <div class="output">
            <h3>Uploaded Content</h3>
            <div class="hex-view" id="uploadedContent">
                <div>Address</div>
                <script>
                    for (let i = 0; i < 16; i++) {
                        document.write(`<div>${i.toString(16).toUpperCase().padStart(2, '0')}</div>`);
                    }
                    document.write(`<div>ASCII</div>`);
                </script>
            </div>
        </div>

        <div class="output">
            <h3>Flash Content</h3>
            <div class="hex-view" id="flashContent">
                <div>Address</div>
                <script>
                    for (let i = 0; i < 16; i++) {
                        document.write(`<div>${i.toString(16).toUpperCase().padStart(2, '0')}</div>`);
                    }
                    document.write(`<div>ASCII</div>`);
                </script>
            </div>
        </div>

        <div id="operation-info" class="info">İşlem bilgisi burada görüntülenecek...</div>
    </div>

    <script>
        const sectorSelect = document.getElementById("sector");
        const blockSelect = document.getElementById("block");
        const uploadedContent = document.getElementById("uploadedContent");
        const flashContent = document.getElementById("flashContent");
        const selectFile = document.getElementById("selectFile");
        const uploadButton = document.getElementById("upload");
        const readButton = document.getElementById("read");

        function updateAddressRange() {
            const sector = parseInt(sectorSelect.value);
            const block = parseInt(blockSelect.value);

            if (!isNaN(sector) && !isNaN(block)) {
                const startAddress = (block * 16 * 4096) + (sector * 4096);
                const endAddress = startAddress + 4096 - 1;

                const hexView = (start, end) => {
                    let html = `<div>Address</div>`;
                    for (let i = 0; i < 16; i++) {
                        html += `<div>${i.toString(16).toUpperCase().padStart(2, '0')}</div>`;
                    }
                    html += `<div>ASCII</div>`;
                    for (let addr = start; addr <= end; addr += 16) {
                        html += `<div>${addr.toString(16).toUpperCase().padStart(6, '0')}</div>`;
                        for (let j = 0; j < 16; j++) {
                            html += `<div>--</div>`;
                        }
                        html += `<div>....</div>`;
                    }
                    return html;
                };

                uploadedContent.innerHTML = hexView(startAddress, endAddress);
                flashContent.innerHTML = hexView(startAddress, endAddress);
            }
        }

        function updateOperationInfo(message) {
          const operationInfoBox = document.getElementById('operation-info');
          operationInfoBox.innerText = message;
        }

        sectorSelect.addEventListener("change", updateAddressRange);
        blockSelect.addEventListener("change", updateAddressRange);
        let fileContent = "";

        uploadButton.addEventListener("click", () => {
            const file = selectFile.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = function(event) {
                    fileContent = event.target.result;
                    const sector = parseInt(sectorSelect.value);
                    const block = parseInt(blockSelect.value);

                    fetch(`/upload`, {
                      method: 'POST',
                      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                      body: "content=" + encodeURIComponent(uploadedContent)
                    }).then(response => response.text());

                    if (!isNaN(sector) && !isNaN(block)) {
                        const startAddress = (block * 16 * 4096) + (sector * 4096);
                        const hexData = fileContent
                            .split('')
                            .map((char, index) => ({
                                hex: char.charCodeAt(0).toString(16).padStart(2, '0'),
                                ascii: /[\x20-\x7E]/.test(char) ? char : '.'
                            }));

                        let html = `<div>Address</div>`;
                        for (let i = 0; i < 16; i++) {
                            html += `<div>${i.toString(16).toUpperCase().padStart(2, '0')}</div>`;
                        }
                        html += `<div>ASCII</div>`;

                        for (let addr = 0; addr < hexData.length; addr += 16) {
                            html += `<div>${(startAddress + addr).toString(16).toUpperCase().padStart(6, '0')}</div>`;
                            for (let j = 0; j < 16; j++) {
                                if (hexData[addr + j]) {
                                    html += `<div>${hexData[addr + j].hex}</div>`;
                                } else {
                                    html += `<div>--</div>`;
                                }
                            }
                            html += `<div>${hexData.slice(addr, addr + 16).map(x => x.ascii).join('')}</div>`;
                        }

                        uploadedContent.innerHTML = html;
                    }
                };
                reader.readAsText(file);
            }
        });

        async function handleAction(action) {
          const sector = parseInt(sectorSelect.value);
          const block = parseInt(blockSelect.value);

          if (action === "write") {
            if (!uploadedContent) {
              updateOperationInfo("Please upload a file first.");
              return;
            }
            await fetch(`/write?sector=${sector}&block=${block}`, {
              method: 'POST',
              headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
              body: "content=" + encodeURIComponent(fileContent)
            }).then(response => response.text())
              .then(text => updateOperationInfo(text));
          } else if (action === "read") {
            const sector = parseInt(sectorSelect.value);
            const block = parseInt(blockSelect.value);

            const response = await fetch(`/read?sector=${sector}&block=${block}`);
            const flashText = await response.text();
            updateOperationInfo("Flash content read successfully.");

            if (!isNaN(sector) && !isNaN(block)) {
                const startAddress = (block * 16 * 4096) + (sector * 4096);
                const hexData = flashText
                    .split('')
                    .map((char, index) => ({
                        hex: char.charCodeAt(0).toString(16).padStart(2, '0'),
                        ascii: /[\x20-\x7E]/.test(char) ? char : '.'
                    }));

                let html = `<div>Address</div>`;
                for (let i = 0; i < 16; i++) {
                    html += `<div>${i.toString(16).toUpperCase().padStart(2, '0')}</div>`;
                }
                html += `<div>ASCII</div>`;

                for (let addr = 0; addr < hexData.length; addr += 16) {
                    html += `<div>${(startAddress + addr).toString(16).toUpperCase().padStart(6, '0')}</div>`;
                    for (let j = 0; j < 16; j++) {
                        if (hexData[addr + j]) {
                            html += `<div>${hexData[addr + j].hex}</div>`;
                        } else {
                            html += `<div>--</div>`;
                        }
                    }
                    html += `<div>${hexData.slice(addr, addr + 16).map(x => x.ascii).join('')}</div>`;
                }

                flashContent.innerHTML = html;
            }
          } else if (action === "deletesector") {
            const response = await fetch(`/deletesector?sector=${sector}&block=${block}`);
            const text = await response.text();
            updateOperationInfo(text);
          } else if (action === "deleteblock") {
            const response = await fetch(`/deleteblock?block=${block}`);
            const text = await response.text();
            updateOperationInfo(text);
          } else if (action === "blank") {
            const response = await fetch(`/blank?sector=${sector}&block=${block}`);
            const text = await response.text();
            updateOperationInfo(text);
          } else if (action === "verify") {
            const response = await fetch(`/verify?sector=${sector}&block=${block}`);
            const text = await response.text();
            updateOperationInfo(text);
          } else if (action === "jedec") {
            const response = await fetch(`/jedec`);
            const text = await response.text();
            updateOperationInfo(text);
          }
        }
    </script>
</body>
</html>
)rawliteral";

String uploadedContent = "";

// Verilen içeriği Flash'e yazma
bool writeToFlash(uint32_t address, const String& content) 
{
    size_t contentLength = content.length();
    for (size_t i = 0; i < contentLength; i += 256) {
        size_t chunkSize = min(contentLength - i, (size_t)256);
        flashChip.writeData(address + (uint32_t)i, (const uint8_t*)content.c_str() + (uint32_t)i, chunkSize);
    }
    Serial.println("Flash'e yazma işlemi tamamlandı.");
    return true;
}

// Flash sector silme
bool deleteSectorFromFlash(uint32_t address)
{
  flashChip.eraseSector(address);
  return true;
}

// Flash block silme
bool deleteBlockFromFlash(uint32_t address)
{
  flashChip.eraseBlock(address);
  return true;
}

void setup() {
    Serial.begin(115200);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize the flash chip
    flashChip.begin();

    testAddressing(flashChip);
    disableWriteProtection(flashChip);
    testChipErase(flashChip);
    testSectorErase(flashChip, 0x000000);
    testVerify(flashChip);

    // Serve the HTML page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", htmlPage);
    });

    // Handle file upload
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("content", true)) 
        {
            uploadedContent = request->getParam("content", true)->value();
            if (!uploadedContent.isEmpty()) 
            {
                request->send(200, "text/plain", "İçerik yüklendi.");
            } else 
            {
                request->send(500, "text/plain", "İçerik yüklenemedi.");
            }
        } 
        else 
        {
            request->send(400, "text/plain", "İçerik eksik.");
        }
    });

    // Handle write operation
    server.on("/write", HTTP_POST, [](AsyncWebServerRequest *request){
        String sector = request->getParam("sector")->value();
        String block = request->getParam("block")->value();
        uint32_t address = (block.toInt() * 16 * 4096) + (sector.toInt() * 4096);
        if (request->hasParam("content", true)) 
        {
          uploadedContent = request->getParam("content", true)->value();
          if (writeToFlash(address, uploadedContent)) 
          {
            request->send(200, "text/plain", "Flash'e yazıldı.");
          } 
          else 
          {
            request->send(500, "text/plain", "Flash'e yazılamadı.");
          }
        } 
        else 
        {
            request->send(400, "text/plain", "İçerik eksik.");
        }
    });

    // Handle read operation
    server.on("/read", HTTP_GET, [](AsyncWebServerRequest *request){
        String sector = request->getParam("sector")->value();
        String block = request->getParam("block")->value();
        uint32_t flashAddress = (block.toInt() * 16 * 4096) + (sector.toInt() * 4096);
        uint8_t buffer[256];
        String content = "";
        for (uint32_t address = flashAddress; address < 4096; address += 256) {
          flashChip.readData(address, buffer, sizeof(buffer));
          for (size_t i = 0; i < sizeof(buffer); i++) 
          {
              if (buffer[i] == 0xFF) break;
              content += (char)buffer[i];
          }
        }
        Serial.println("Flash'ten okuma işlemi tamamlandı.");
        request->send(200, "text/plain", content);
    });

    // Handle deletesector operation
    server.on("/deletesector", HTTP_GET, [](AsyncWebServerRequest *request){
        String sector = request->getParam("sector")->value();
        String block = request->getParam("block")->value();
        uint32_t address = (block.toInt() * 16 * 4096) + (sector.toInt() * 4096);

        if (deleteSectorFromFlash(address)) 
        {
            request->send(200, "text/plain", "Flash tamamen silindi.");
        } 
        else 
        {
            request->send(500, "text/plain", "Silme işlemi başarısız.");
        }
    });

    // Handle deleteblock operation
    server.on("/deleteblock", HTTP_GET, [](AsyncWebServerRequest *request){
        String block = request->getParam("block")->value();
        uint32_t address = (block.toInt() * 16 * 4096);

        if (deleteBlockFromFlash(address)) {
            request->send(200, "text/plain", "Flash tamamen silindi.");
        } else {
            request->send(500, "text/plain", "Silme işlemi başarısız.");
        }
    });

    // Handle verify operation
    server.on("/verify", HTTP_GET, [](AsyncWebServerRequest *request){
        if (uploadedContent.isEmpty()) 
        {
            request->send(400, "text/plain", "Yüklenen içerik mevcut değil.");
            return;
        }

        String sector = request->getParam("sector")->value();
        String block = request->getParam("block")->value();
        uint32_t address = (block.toInt() * 16 * 4096) + (sector.toInt() * 4096);
        bool result = flashChip.verifyData(address, (const uint8_t*)uploadedContent.c_str(), sizeof((const uint8_t*)uploadedContent.c_str()));
        if (result) 
        {
            request->send(200, "text/plain", "Verify başarılı: Veriler eşleşiyor!");
        } 
        else 
        {
            request->send(200, "text/plain", "Verify başarısız: Veriler eşleşmiyor!");
        }

    });

    // Handle blank check operation
    server.on("/blank", HTTP_GET, [](AsyncWebServerRequest *request){
        String sector = request->getParam("sector")->value();
        String block = request->getParam("block")->value();
        uint32_t address = (block.toInt() * 16 * 4096) + (sector.toInt() * 4096);
        bool isEmpty = flashChip.isBlank(address);
        if (isEmpty) {
            request->send(200, "text/plain", "Flash boş.");
        } else {
            request->send(200, "text/plain", "Flash dolu.");
        }
    });

    // Handle JEDEC ID request
    server.on("/jedec", HTTP_GET, [](AsyncWebServerRequest *request){
        uint32_t jedecId = flashChip.getJEDECID();
        char buffer[50];
        sprintf(buffer, "JEDEC ID: %06X", jedecId);
        request->send(200, "text/plain", buffer);
    });

    server.begin();
}

void loop() {
    
}

void testAddressing(W25Q &flash) {
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

        flash.eraseSector(address); 
        flash.writeData(address, testData, sizeof(testData)); 
        flash.readData(address, readBuffer, sizeof(readBuffer)); 

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

void disableWriteProtection(W25Q &flash) {
    uint8_t status;
    flash.readCommand(0x05, &status, 1);

    Serial.print("Koruma durumu (Status Register): ");
    Serial.println(status, BIN);

    uint8_t disableProtection[2] = {0x00, 0x00}; // BP, TB ve SEC bits
    flash.writeEnable(); //  (CMD_WRITE_ENABLE)
    flash.sendCommand(0x01, disableProtection, 2); // Write Status Register
    flash.waitForReady(); 
    if (status & 0x3C) { // if BP bits are set
        Serial.println("Koruma aktif, devre dışı bırakılıyor...");
        uint8_t disableProtection[2] = {0x01, 0x00}; // reset the Status register
        flash.sendCommand(0x01, disableProtection, 2); // Write Status Register command
        flash.waitForReady();
        Serial.println("Koruma devre dışı bırakıldı.");
    } else {
        Serial.println("Koruma zaten devre dışı.");
    }
}

void testChipErase(W25Q &flash) {
    flash.chipErase();
    Serial.println("Çip tamamen silindi. Kontrol ediliyor...");
    bool isBlank = flash.isBlank(0x000000); 
    if (isBlank) {
        Serial.println("Çip tamamen boş.");
    } else {
        Serial.println("Çip silinemedi!");
    }
}

void testSectorErase(W25Q &flash, uint32_t address) {
    flash.eraseSector(address);
    Serial.print("Sektör silindi. Kontrol ediliyor: ");
    Serial.println(address, HEX);
    bool isBlank = flash.isBlank(address);
    if (isBlank) {
        Serial.println("Sektör tamamen boş.");
    } else {
        Serial.println("Sektör silinemedi!");
    }
}

void testVerify(W25Q &flash) {
    uint8_t testData[256];
    for (int i = 0; i < 256; i++) {
        testData[i] = i; 
    }

    uint32_t address = 0x000000;
    flash.writeData(address, testData, sizeof(testData));

    bool isVerified = flash.verifyData(address, testData, sizeof(testData));
    if (isVerified) {
        Serial.println("Doğrulama başarılı!");
    } else {
        Serial.println("Doğrulama başarısız!");
    }
}