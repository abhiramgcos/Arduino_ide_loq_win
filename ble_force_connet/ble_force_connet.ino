#include <NimBLEDevice.h>

static NimBLEAddress targetAddr("13:26:aa:41:b7:d3", BLE_ADDR_RANDOM);
static bool done = false;

class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* d) {  // NO override keyword
    if (d->getAddress() == targetAddr) {
      Serial.printf("Found FT-1603A! RSSI: %d | Connectable: %s\n",
        d->getRSSI(), d->isConnectable() ? "YES" : "NO");

      if (d->isConnectable() && !done) {
        NimBLEDevice::getScan()->stop();
        NimBLEClient* pClient = NimBLEDevice::createClient();
        if (pClient->connect(d)) {
          Serial.println("Connected! Deleting bond...");
          NimBLEDevice::deleteBond(d->getAddress());
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
  pScan->setAdvertisedDeviceCallbacks(new ScanCallbacks()); // v1.x method
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);
}

void loop() {
  if (!done) {
    Serial.println("Scanning...");
    NimBLEDevice::getScan()->start(4, false);
    NimBLEDevice::getScan()->clearResults();
    delay(500);
  }
}
