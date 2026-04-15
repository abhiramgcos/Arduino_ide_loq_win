#include <NimBLEDevice.h>

static NimBLEAddress targetAddr("13:26:aa:41:b7:d3", BLE_ADDR_RANDOM);
static bool done = false;

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    if (d->getAddress() == targetAddr) {
      Serial.printf("Found FT-1603A! RSSI: %d | Connectable: %s\n",
        d->getRSSI(), d->isConnectable() ? "YES" : "NO");

      if (d->isConnectable() && !done) {
        NimBLEDevice::getScan()->stop();
        NimBLEClient* pClient = NimBLEDevice::createClient();
        if (pClient->connect(d)) {
          Serial.println("Connected! Deleting bond...");
          NimBLEDevice::deleteBond(targetAddr);
          pClient->disconnect();
          Serial.println("Done! Power cycle the keyboard now.");
          done = true;
        } else {
          Serial.println("Connect failed. Retrying...");
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
  pScan->setScanCallbacks(new ScanCallbacks(), false); // v2.x method
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);
  pScan->setMaxResults(0); // don't buffer results, use callback only
}

void loop() {
  if (!done) {
    Serial.println("Scanning...");
    NimBLEDevice::getScan()->start(4000); // v2.x uses milliseconds, not seconds
    delay(500);
  }
}
