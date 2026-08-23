# CubeX-48
## 4x3x4 Volumetric 3D LED Matrix & Mobile BLE Controller

An open-source, 48-voxel volumetric 3D LED cube driven by an ESP32 microcontroller with a real-time Flutter mobile control interface over Bluetooth Low Energy (BLE).

Developed and prototyped as a hardware-software embedded systems project during the Onam vacation break.

---

## 👨‍💻 Connect With Me

- **LinkedIn:** [Paulwin.T.P - LinkedIn Profile](www.linkedin.com/in/paulwin-t-p)
- **Project Repository:** [GitHub Repository]( https://github.com/PaulwinTP/CubeX-48.git)

---

## 📱 Project Demo Video

<p align="center">
  <!-- Portrait video preview embed -->
  <video src="assets/demo_video.mp4" width="320" height="640" controls muted autoplay loop style="border-radius: 16px; box-shadow: 0 4px 12px rgba(0,0,0,0.15);">
    Your browser does not support the video tag.
  </video>
  <br>
  <em>Figure 1: Full system demonstration showing BLE discovery, real-time animation switching, and manual voxel control.</em>
</p>


---

## 🧊 3D Coordinate Topology & Architecture

The cube is structured as a 3D coordinate system: **$X$ (Columns: $0 \dots 3$)**, **$Y$ (Rows/Depth: $0 \dots 2$)**, and **$Z$ (Layers/Height: $0 \dots 3$)**, creating a total of $4 \times 3 \times 4 = 48$ individually addressable spatial voxels.

```text
                   Z (Height)
                   ▲
                   │
                   │    Layer 3 [Top]
                   │    Layer 2 [Mid-Top]
                   │    Layer 1 [Mid-Bottom]
                   │    Layer 0 [Base Floor]
                   │
                   └────────────────────────► X (Width: 0 to 3)
                  /
                 /
                /  Y (Depth: 0 to 2)
               ▼

          [0,2,3] ─── [1,2,3] ─── [2,2,3] ─── [3,2,3]   <-- Layer 3
            │ \         │ \         │ \         │ \
            │  [0,1,3] ─┼───[1,1,3] ─┼───[2,1,3] ─┼─── [3,1,3]
            │    │ \    │     │ \   │     │ \   │     │ \
            │    │  [0,0,3] ──┼───[1,0,3] ──┼───[2,0,3] ── [3,0,3]
            │    │    │ │     │   │ │     │   │ │     │   │
          [0,2,0] ─── [1,2,0] ─── [2,2,0] ─── [3,2,0]   <-- Layer 0 (Base)
              \  │        \   │       \   │       \   │
               \ [0,1,0] ──┼──[1,1,0] ──┼──[2,1,0] ──┼── [3,1,0]
                 \   │       \    │      \    │      \    │
                  [0,0,0] ─── [1,0,0] ─── [2,0,0] ─── [3,0,0]
```
### Physical Multiplexing Principles
* **Common Anodes (Vertical Columns):** 12 vertical rigid wires connect the positive terminals (anodes) of all 4 LEDs in each column ($Z=0 \dots 3$).
* **Common Cathodes (Horizontal Layers):** 4 horizontal grid frames tie the negative terminals (cathodes) of all 12 LEDs on each height layer together.
* **Direct GPIO Active-LOW Sinking:** ESP32 GPIO pins act as grounds by pulling active cathode layers to `LOW` (0V). Inactive layers are held at `HIGH` (3.3V) to prevent reverse conduction.
* **Persistence of Vision (PoV):** The ESP32 scans each horizontal layer sequentially at ~100 Hz on Core 0 with blanking phases, creating flicker-free and ghost-free 3D imagery.

---

## ⚡ Complete Hardware Wiring & Pinout
```text
                ┌─────────────────────────────────────────┐
                │             ESP32 DevKit V1             │
                └────┬──────┬──────┬──────┬─────────┬─────┘
                     │      │      │      │         │
    LAYER CATHODES   │      │      │      │         │  COLUMN ANODES (x12)
    (Active LOW)     │      │      │      │         │  (Active HIGH)
    Layer 0 (Base)  ─┴─ GPIO 25    │      │         ├─ GPIO 4   ─► Col 0 (X=0, Y=0)
    Layer 1 (Mid-L) ─── GPIO 26 ───┘      │         ├─ GPIO 5   ─► Col 1 (X=1, Y=0)
    Layer 2 (Mid-H) ─── GPIO 27 ──────────┘         ├─ GPIO 13  ─► Col 2 (X=2, Y=0)
    Layer 3 (Top)   ─── GPIO 32 ────────────────────┼─ GPIO 14  ─► Col 3 (X=3, Y=0)
                                                    ├─ GPIO 15  ─► Col 4 (X=0, Y=1)
                                                    ├─ GPIO 16  ─► Col 5 (X=1, Y=1)
                                                    ├─ GPIO 17  ─► Col 6 (X=2, Y=1)
                                                    ├─ GPIO 18  ─► Col 7 (X=3, Y=1)
                                                    ├─ GPIO 19  ─► Col 8 (X=0, Y=2)
                                                    ├─ GPIO 21  ─► Col 9 (X=1, Y=2)
                                                    ├─ GPIO 22  ─► Col 10 (X=2, Y=2)
                                                    └─ GPIO 23  ─► Col 11 (X=3, Y=2)
```
### Layer Pins (Direct Cathode Sinks)
| Layer Index | Physical Plane | ESP32 Pin | Active Logic |
| :--- | :--- | :--- | :--- |
| **Layer 0** | Base Floor | `GPIO 25` | `LOW` (0V) = Active |
| **Layer 1** | Mid-Lower | `GPIO 26` | `LOW` (0V) = Active |
| **Layer 2** | Mid-Upper | `GPIO 27` | `LOW` (0V) = Active |
| **Layer 3** | Top Floor | `GPIO 32` | `LOW` (0V) = Active |

### Column Pins (Vertical Anode Sources)
| Column Index | Matrix Coordinate $(X, Y)$ | ESP32 Pin | Logic |
| :--- | :--- | :--- | :--- |
| **Col 0** | Front Row $(X=0, Y=0)$ | `GPIO 4` | `HIGH` (3.3V) = Active |
| **Col 1** | Front Row $(X=1, Y=0)$ | `GPIO 5` | `HIGH` (3.3V) = Active |
| **Col 2** | Front Row $(X=2, Y=0)$ | `GPIO 13` | `HIGH` (3.3V) = Active |
| **Col 3** | Front Row $(X=3, Y=0)$ | `GPIO 14` | `HIGH` (3.3V) = Active |
| **Col 4** | Middle Row $(X=0, Y=1)$ | `GPIO 15` | `HIGH` (3.3V) = Active |
| **Col 5** | Middle Row $(X=1, Y=1)$ | `GPIO 16 (RX2)` | `HIGH` (3.3V) = Active |
| **Col 6** | Middle Row $(X=2, Y=1)$ | `GPIO 17 (TX2)` | `HIGH` (3.3V) = Active |
| **Col 7** | Middle Row $(X=3, Y=1)$ | `GPIO 18` | `HIGH` (3.3V) = Active |
| **Col 8** | Back Row $(X=0, Y=2)$ | `GPIO 19` | `HIGH` (3.3V) = Active |
| **Col 9** | Back Row $(X=1, Y=2)$ | `GPIO 21` | `HIGH` (3.3V) = Active |
| **Col 10** | Back Row $(X=2, Y=2)$ | `GPIO 22` | `HIGH` (3.3V) = Active |
| **Col 11** | Back Row $(X=3, Y=2)$ | `GPIO 23` | `HIGH` (3.3V) = Active |

---

## 🔄 System Architecture & Workflow
```text
  ┌─────────────────────────────────────────────────────────────┐
  │                    FLUTTER MOBILE CLIENT                    │
  │  - Sky-Blue UI Theme (#0288D1)                              │
  │  - Reactive BLE Discovery (Service: 6E400001...)            │
  │  - 20 Preset Animation Cards + Dynamic Speed Slider         │
  │  - 48-Node Spatial Interactive Grid with Duration Timers    │
  └──────────────────────────────┬──────────────────────────────┘
                                 │
                     Bluetooth Low Energy (GATT)
                     UART Command Packets:
                     "MODE:<ID>", "SPEED:<MS>", "LED:X,Y,Z,T"
                                 │
                                 ▼
  ┌─────────────────────────────────────────────────────────────┐
  │                     ESP32 DUAL-CORE SOC                     │
  │                                                             │
  │   CORE 1: System & Connectivity   CORE 0: Display Engine    │
  │   ┌───────────────────────────┐   ┌───────────────────────┐ │
  │   │ - BLE Server Callbacks    │   │ - FreeRTOS Task Loop  │ │
  │   │ - Command String Parser   │   │ - 100 Hz Multiplexer  │ │
  │   │ - 20 Animation State Mach │   │ - Anti-Ghost Blanking │ │
  │   │ - Timed Voxel Scheduler   │   │ - Direct GPIO Sink    │ │
  │   └─────────────┬─────────────┘   └───────────▲───────────┘ │
  │                 │                             │             │
  │                 └────── Shared Framebuffer ───┘             │
  │                         uint8_t cube[4][3][4]               │
  └─────────────────────────────────────────────────────────────┘
                                 │
                           GPIO Signals
                                 │
                                 ▼
                     4x3x4 3D LED CUBE HARDWARE

```
## 📡 Bluetooth Low Energy (BLE) Interface

* **Device Name:** `ESP32_3D_CUBE`
* **Service UUID:** `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
* **RX Characteristic UUID:** `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` (Write / Write Without Response)
* **TX Characteristic UUID:** `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` (Notify)

### Command Set
| Command Format | Example | Action Executed on ESP32 |
| :--- | :--- | :--- |
| `MODE:<1-20>` | `MODE:2` | Switches active animation engine to **Realistic Fire** |
| `MODE:3` | `MODE:3` | Displays solid volumetric **3D Letter "H"** across Layers 0-2 |
| `MODE:18` | `MODE:18` | Renders **3-Tier Geometric Pyramid** |
| `MODE:20` | `MODE:20` | All LEDs off (Standby mode) |
| `SPEED:<ms>` | `SPEED:60` | Sets animation tick rate to $60\,\text{ms}$ |
| `LED:<X>,<Y>,<Z>,<T>` | `LED:0,0,3,2000` | Illuminates voxel at $(0,0,3)$ for exactly $2000\,\text{ms}$ |

---

## 🎭 Included 20 Animation Library

1. **Diagonal Corner Wave:** Slices across diagonal planes $(X+Y+Z=\text{dist})$ from $(0,0,0)$ to $(3,2,3)$.
2. **Realistic Fire & Embers:** Stochastic fuel ignition on Layer 0 with ascending heat dissipation and sparks.
3. **Volumetric 3D Letter "H":** Dual 3-layer vertical pillars interconnected by a center bridge on Layer 1.
4. **Digital Rain:** Falling vertical streaks reminiscent of the Matrix stream.
5. **Traveling Sine Wave:** Sinusoidal surface wave traversing the X-axis.
6. **Perimeter Spiral Vortex:** Continuous helical motion following the 10-node outer perimeter.
7. **Expanding/Collapsing Box:** Volumetric pulsing from central core $(1,1,1)$ to outer boundaries.
8. **Layer Bounce Scanner:** Horizontal sheet moving cyclically up and down between Layers 0 and 3.
9. **X-Plane Sweep:** Vertical sheet scanning side-to-side across the X-axis.
10. **Y-Plane Sweep:** Vertical sheet scanning front-to-back across depth planes.
11. **Propeller Spin:** Rotating 3D plane pivoting along the central vertical Z-axis.
12. **Twinkle Stars:** Random 3D spatial sparkle generator with stochastic decay.
13. **8-Corner Orbit:** Sequential chaser traversing all 8 extreme corners of the bounding cube.
14. **Pulsing Core:** Internal $2\times1\times2$ cluster breathing outwards.
15. **Helium Bubbles:** Buoyancy simulation with randomly generated rising voxels.
16. **Volumetric 3D Cross:** Two intersecting diagonal planar sheets creating an 'X' pattern.
17. **Flash Strobe:** High-contrast alternating full-cube flash burst.
18. **3-Tier Geometric Pyramid:** 12-LED base (L0), 6-LED mid-tier (L1), and 2-LED apex peak (L2).
19. **Sequential Snake Coil:** Full 48-voxel serial path traversal across all layers.
20. **Standby Mode:** Framebuffer clear and blanking.

---

## 🚀 Setup & Execution Guide

### 1. Upload ESP32 Firmware
1. Open the Arduino IDE.
2. Install the **ESP32 Board Package** via Boards Manager (`esp32` by Espressif Systems).
3. Select **Board:** `ESP32 Dev Module`.
4. Copy the C++ firmware source code into the editor.
5. Compile and upload over USB at `115200` baud.

### 2. Install the Mobile App
1. Download the pre-built application APK from the **[Releases](https://github.com/your-username/esp32-3d-led-cube/releases)** section (or install the provided APK on your Android device).
2. Open the app and grant **Bluetooth** and **Location** permissions when prompted.

### 3. Connect & Control
1. Power on the ESP32 3D Cube.
2. Launch the app and tap the **Radar Scan** button on the Connect tab.
3. Select **`ESP32_3D_CUBE`** to establish a connection.
4. Seamlessly trigger any of the **20 preset 3D animations**, adjust the speed slider, or switch to the **Manual Voxel** tab to illuminate individual $(X, Y, Z)$ nodes with custom hold timers.
