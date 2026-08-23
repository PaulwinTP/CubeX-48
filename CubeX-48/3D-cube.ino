#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

const int NUM_LAYERS = 4;
const int NUM_COLS   = 12;

// Layers (Direct Cathodes): Layer 0 (Bottom) -> Layer 3 (Top) [Active LOW]
const int layerPins[4] = {25, 26, 27, 32};

// Columns (Direct Anodes): Col 0 -> Col 11 [Active HIGH]
// (x, y) mapping: idx = y * 4 + x
const int colPins[12]  = {4, 5, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23};

uint8_t cube[4][3][4] = {0};
volatile uint8_t activeMode = 1;
volatile int animSpeed = 100;

struct ManualVoxel {
  int x, y, z;
  unsigned long durationMs;
  unsigned long startTime;
  bool active;
} manualLeds[48];

// --- Core Helper Functions ---

void setVoxel(int x, int y, int z, uint8_t state) {
  if (x >= 0 && x < 4 && y >= 0 && y < 3 && z >= 0 && z < 4) {
    cube[z][y][x] = state;
  }
}

void clearCube() {
  memset(cube, 0, sizeof(cube));
}

void fillCube() {
  memset(cube, 1, sizeof(cube));
}

// Display Multiplexer with Current-Protection Dwell Balancing
void multiplexTask(void *pvParameters) {
  for (int i = 0; i < NUM_LAYERS; i++) { 
    pinMode(layerPins[i], OUTPUT); 
    digitalWrite(layerPins[i], HIGH); // HIGH = OFF
  }
  for (int i = 0; i < NUM_COLS; i++) { 
    pinMode(colPins[i], OUTPUT); 
    digitalWrite(colPins[i], LOW); // LOW = OFF
  }

  while (true) {
    for (int z = 0; z < NUM_LAYERS; z++) {
      // 1. Blanking: Deactivate all layers
      for (int l = 0; l < NUM_LAYERS; l++) {
        digitalWrite(layerPins[l], HIGH);
      }

      // 2. Set Column States for layer z
      int colIdx = 0;
      for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 4; x++) {
          digitalWrite(colPins[colIdx++], cube[z][y][x] ? HIGH : LOW);
        }
      }

      // 3. Drive Active Layer Cathode LOW
      digitalWrite(layerPins[z], LOW);
      delayMicroseconds(2500); // Stable hold time

      // 4. Blank again
      digitalWrite(layerPins[z], HIGH);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// --- BLE Callbacks ---

class BLECallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue().c_str();
    if (rxValue.length() > 0) {
      Serial.print("[BLE CMD] ");
      Serial.println(rxValue);

      if (rxValue.startsWith("MODE:")) {
        activeMode = rxValue.substring(5).toInt();
        clearCube();
      } else if (rxValue.startsWith("SPEED:")) {
        animSpeed = rxValue.substring(6).toInt();
      } else if (rxValue.startsWith("LED:")) {
        int x, y, z, dur;
        if (sscanf(rxValue.c_str(), "LED:%d,%d,%d,%d", &x, &y, &z, &dur) == 4) {
          if (activeMode != 99) {
            clearCube();
            activeMode = 99;
          }
          setVoxel(x, y, z, 1);
          int idx = z * 12 + y * 4 + x;
          if (idx >= 0 && idx < 48) {
            manualLeds[idx] = {x, y, z, (unsigned long)dur, millis(), true};
          }
        }
      }
    }
  }
};

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("[BLE] Mobile Connected!");
  }
  void onDisconnect(BLEServer* pServer) {
    Serial.println("[BLE] Disconnected. Restarting Advertising...");
    pServer->getAdvertising()->start();
  }
};

// --- Animations Engine ---

void runAnimation(int mode) {
  static int step = 0;
  static unsigned long lastUpdate = 0;

  if (mode == 99) {
    unsigned long now = millis();
    for (int i = 0; i < 48; i++) {
      if (manualLeds[i].active && (now - manualLeds[i].startTime >= manualLeds[i].durationMs)) {
        setVoxel(manualLeds[i].x, manualLeds[i].y, manualLeds[i].z, 0);
        manualLeds[i].active = false;
      }
    }
    return;
  }

  if (millis() - lastUpdate < (unsigned long)animSpeed) return;
  lastUpdate = millis();

  switch (mode) {
    case 1: { // 1. Diagonal Corner Wave
      clearCube();
      for (int z = 0; z < 4; z++)
        for (int y = 0; y < 3; y++)
          for (int x = 0; x < 4; x++)
            if (x + y + z == (step % 9)) setVoxel(x, y, z, 1);
      step++;
      break;
    }
    case 2: { // 2. Realistic Fire
      clearCube();
      for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 3; y++) {
          if (random(0, 100) < 90) setVoxel(x, y, 0, 1);
          if (cube[0][y][x] && random(0, 100) < 65) setVoxel(x, y, 1, 1);
          if (cube[1][y][x] && random(0, 100) < 35) setVoxel(x, y, 2, 1);
          if (cube[2][y][x] && random(0, 100) < 15) setVoxel(x, y, 3, 1);
        }
      }
      break;
    }
    case 3: { // 3. Precision 3D Letter "H"
      clearCube();
      // Pillars across Layers 0, 1, and 2
      for (int z = 0; z < 3; z++) {
        // Left Column (Front, Center, Back)
        setVoxel(0, 0, z, 1);
        setVoxel(0, 1, z, 1);
        setVoxel(0, 2, z, 1);

        // Right Column (Front, Center, Back)
        setVoxel(3, 0, z, 1);
        setVoxel(3, 1, z, 1);
        setVoxel(3, 2, z, 1);
      }
      // Horizontal Crossbar bridge through depth on Layer 1
      for (int y = 0; y < 3; y++) {
        setVoxel(1, y, 1, 1);
        setVoxel(2, y, 1, 1);
      }
      break;
    }
    case 4: { // 4. Matrix Digital Rain
      for (int z = 0; z < 3; z++)
        for (int y = 0; y < 3; y++)
          for (int x = 0; x < 4; x++) cube[z][y][x] = cube[z+1][y][x];
      for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++) cube[3][y][x] = (random(0, 10) > 7) ? 1 : 0;
      break;
    }
    case 5: { // 5. Sine Wave
      clearCube();
      for (int x = 0; x < 4; x++) {
        int z = (x + step) % 4;
        for (int y = 0; y < 3; y++) setVoxel(x, y, z, 1);
      }
      step++;
      break;
    }
    case 6: { // 6. Perimeter Spiral Vortex
      const int perim[10][2] = {{0,0},{1,0},{2,0},{3,0},{3,1},{3,2},{2,2},{1,2},{0,2},{0,1}};
      clearCube();
      for (int z = 0; z < 4; z++) {
        int idx = (step + z * 2) % 10;
        setVoxel(perim[idx][0], perim[idx][1], z, 1);
      }
      step++;
      break;
    }
    case 7: { // 7. Box Pulse
      clearCube();
      if ((step % 2) == 0) {
        setVoxel(1, 1, 1, 1); setVoxel(2, 1, 1, 1);
        setVoxel(1, 1, 2, 1); setVoxel(2, 1, 2, 1);
      } else {
        fillCube();
        setVoxel(1, 1, 1, 0); setVoxel(2, 1, 1, 0);
        setVoxel(1, 1, 2, 0); setVoxel(2, 1, 2, 0);
      }
      step++;
      break;
    }
    case 8: { // 8. Z-Layer Bounce Scanner
      clearCube();
      int zSeq[6] = {0, 1, 2, 3, 2, 1};
      int currZ = zSeq[step % 6];
      for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++) setVoxel(x, y, currZ, 1);
      step++;
      break;
    }
    case 9: { // 9. X-Plane Sweep
      clearCube();
      int xSeq[6] = {0, 1, 2, 3, 2, 1};
      int currX = xSeq[step % 6];
      for (int z = 0; z < 4; z++)
        for (int y = 0; y < 3; y++) setVoxel(currX, y, z, 1);
      step++;
      break;
    }
    case 10: { // 10. Y-Plane Sweep
      clearCube();
      int ySeq[4] = {0, 1, 2, 1};
      int currY = ySeq[step % 4];
      for (int z = 0; z < 4; z++)
        for (int x = 0; x < 4; x++) setVoxel(x, currY, z, 1);
      step++;
      break;
    }
    case 11: { // 11. Propeller Spin
      clearCube();
      int st = step % 4;
      for (int z = 0; z < 4; z++) {
        if (st == 0)      { for(int y=0; y<3; y++){ setVoxel(1,y,z,1); setVoxel(2,y,z,1); } }
        else if (st == 1) { setVoxel(0,0,z,1); setVoxel(1,1,z,1); setVoxel(2,1,z,1); setVoxel(3,2,z,1); }
        else if (st == 2) { for(int x=0; x<4; x++){ setVoxel(x,1,z,1); } }
        else if (st == 3) { setVoxel(3,0,z,1); setVoxel(2,1,z,1); setVoxel(1,1,z,1); setVoxel(0,2,z,1); }
      }
      step++;
      break;
    }
    case 12: { // 12. Twinkle Stars
      clearCube();
      for (int i = 0; i < 6; i++) setVoxel(random(0,4), random(0,3), random(0,4), 1);
      break;
    }
    case 13: { // 13. 8-Corner Orbit
      clearCube();
      int cx[8] = {0,3,3,0,0,3,3,0};
      int cy[8] = {0,0,2,2,0,0,2,2};
      int cz[8] = {0,0,0,0,3,3,3,3};
      setVoxel(cx[step % 8], cy[step % 8], cz[step % 8], 1);
      step++;
      break;
    }
    case 14: { // 14. Pulsing Core
      clearCube();
      int st = step % 3;
      if (st == 0)      { setVoxel(1,1,1,1); setVoxel(2,1,1,1); }
      else if (st == 1) { setVoxel(1,1,2,1); setVoxel(2,1,2,1); }
      else              { fillCube(); }
      step++;
      break;
    }
    case 15: { // 15. Helium Bubbles
      for (int z = 3; z > 0; z--)
        for (int y = 0; y < 3; y++)
          for (int x = 0; x < 4; x++) cube[z][y][x] = cube[z-1][y][x];
      for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++) cube[0][y][x] = (random(0, 10) > 8) ? 1 : 0;
      break;
    }
    case 16: { // 16. True 3D Cross (Volumetric X)
      clearCube();
      for (int y = 0; y < 3; y++) {
        // Diagonal 1 (\)
        setVoxel(0, y, 3, 1);
        setVoxel(1, y, 2, 1);
        setVoxel(2, y, 1, 1);
        setVoxel(3, y, 0, 1);

        // Diagonal 2 (/)
        setVoxel(0, y, 0, 1);
        setVoxel(1, y, 1, 1);
        setVoxel(2, y, 2, 1);
        setVoxel(3, y, 3, 1);
      }
      break;
    }
    case 17: { // 17. High-Contrast Flash Strobe
      if ((step % 2) == 0) {
        fillCube();
      } else {
        clearCube();
      }
      step++;
      break;
    }
    case 18: { // 18. True 3-Tier Pyramid
      clearCube();
      // Layer 0: Full Base (All 12 LEDs)
      for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 4; x++) {
          setVoxel(x, y, 0, 1);
        }
      }
      // Layer 1: Mid Tier (Inner 6 LEDs: X=1,2 with all Y)
      for (int y = 0; y < 3; y++) {
        setVoxel(1, y, 1, 1);
        setVoxel(2, y, 1, 1);
      }
      // Layer 2: Apex Peak (Center 2 LEDs: X=1,2 at Y=1)
      setVoxel(1, 1, 2, 1);
      setVoxel(2, 1, 2, 1);

      // Layer 3 remains entirely OFF
      break;
    }
    case 19: { // 19. Snake Coil
      clearCube();
      int flatIdx = step % 48;
      int cz = flatIdx / 12;
      int rem = flatIdx % 12;
      int cy = rem / 4;
      int cx = rem % 4;
      setVoxel(cx, cy, cz, 1);
      step++;
      break;
    }
    case 20: { // 20. All OFF
      clearCube();
      break;
    }
    default:
      clearCube();
      break;
  }
}

// --- Setup & Main Loop ---

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 3D Cube: Direct Drive Calibrated ===");

  BLEDevice::init("ESP32_3D_CUBE");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new BLECallbacks());

  BLECharacteristic *pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Advertising Active!");

  xTaskCreatePinnedToCore(multiplexTask, "MultiplexEngine", 4096, NULL, 1, NULL, 0);
}

void loop() {
  runAnimation(activeMode);
}