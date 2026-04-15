#include <NimBLEDevice.h>

static NimBLEAddress targetAddr("13:26:aa:41:b7:d3", BLE_ADDR_RANDOM);
static bool connected = false;

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(NimBLEAdvertisedDevice* d) override {
    if (d->getAddress() == targetAddr) {
      Serial.printf("Found FT-1603A! RSSI: %d | Connectable: %s\n",
        d->getRSSI(), d->isConnectable() ? "YES" : "NO");

      if (d->isConnectable()) {
        NimBLEDevice::getScan()->stop();
        NimBLEClient* pClient = NimBLEDevice::createClient();
        if (pClient->connect(d)) {
          Serial.println("Connected! Deleting bond...");
          NimBLEDevice::deleteBond(targetAddr);
          pClient->disconnect();
          Serial.println("Done. Power cycle keyboard now!");
          connected = true;
        }
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  NimBLEDevice::init("ESP32Host");
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new ScanCallbacks(), false);
  pScan->setActiveScan(true);
}

void loop() {
  if (!connected) {
    Serial.println("Scanning for FT-1603A...");
    NimBLEDevice::getScan()->start(3, false);
    NimBLEDevice::getScan()->clearResults();
    delay(500);
  }
}
