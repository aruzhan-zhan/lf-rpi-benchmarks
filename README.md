# lf-rpi-benchmarks: Evaluating Lingua Franca Timing Predictability on Raspberry Pi

## Introduction
This repository contains the code and tests used to measure timing predictability and memory interference on a multi-core Raspberry Pi running Linux.

These experiments measure how much a system's timing gets delayed (jitter) when it is constantly bombarded by external hardware interrupts. We compare a standard C program against Lingua Franca to see exactly how much delay is caused when the Raspberry Pi's four physical cores are forced to fight over the same shared memory cache.

### Credits and Departures from Original Work
These benchmarks build directly upon the foundational methodology and library files established in the [programming-pret-machines](https://github.com/magnmaeh/programming-pret-machines) repository. 

While the original tutorial spanned multiple platforms and used a Digilent Analog Discovery tool for waveform generation, this project focuses specifically on the **Raspberry Pi 4B** to see how using multiple processors at the same time affects the system's timing. Key changes from the original work include:
1. **External Signal Generator:** Replacing the expensive Digilent Analog Discovery tool with an **STM32 Nucleo-F302R8** programmed via the Arduino IDE to physically generate strict periodic and sporadic hardware interrupts.
2. **Multi-Core Parallelism:** Introducing a 4-core parallel Lingua Franca benchmark (RPi_Parallel.lf) specifically for the Raspberry Pi. This measures the delays caused when multiple processors fight over shared memory, a test that was absent from the original tutorial.

### ⚠️ Important Dependency Note
Because this project utilizes the core library components of the original research, the Lingua Franca files in this repository rely on relative paths (e.g., `../../lib/RPi/interrupt.cmake`). To compile and run these benchmarks, you **must** clone this repository inside the `experiments/lf/src/` directory of the original `programming-pret-machines` repository.

---

## Repository Structure and File Directory

* **`src/c/main.c`**: The standard C program used as our starting point for comparison. It sets up the pins, waits for the hardware signal, runs the math loop, records the timestamps, and prints the timing delays.
* **`src/lf/RPi_Work.lf`**: This runs the exact same test as the standard C program, but using Lingua Franca on a single core. Because Lingua Franca has to do extra work in the background to manage its strict timing rules, this test lets us measure exactly how much extra delay (overhead) the language itself adds.
* **`src/lf/RPi_Parallel.lf`**: This test runs Lingua Franca across all 4 cores of the Raspberry Pi at the same time. Because all 4 processors have to share the same small part of fast memory (the L2 cache), they end up fighting for access. We use this test to measure the timing delays caused when the processors get in each other's way.
* **`src/metronome/metronome.ino`**: The C++ firmware flashed to the STM32 Nucleo. It acts as the physical interrupt generator, sending 10ms periodic pulses on one pin and randomized sporadic bursts on another.
* **`scripts/normal.py`**: A small Python script that sets up the test. It creates a configuration file (/tmp/config.h) that tells the standard C program exactly how many times to repeat the math loop so we can get a consistent starting measurement.
* **`data/`**: Contains the raw nanosecond timestamp outputs (`*_results.txt`) from the execution of the four benchmarks.
* **`docs/figs/`**: This folder contains the charts and graphs created by the Python script. These images show the timing differences between a single processor and four processors working at the same time.

---

## Hardware Setup

1. Raspberry Pi 4B. It also needs:
   * A Raspberry Pi power supply.
   * A micro-sd card for operating system.
2. STM32 Nucleo-F302R8 (or similar STM32 board): Used as our low-cost signal generator.
   * Micro-USB Cable
3. 3 male-female jumper cables.

Figure 1. Hardware Setup
<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/b7347d8a-da7a-4d1e-a65f-80df11fd5e14" />

### Raspberry Pi 4B Configuration
Since we are running these experiments remotely, we use SSH to control the Raspberry Pi from a laptop. This allows the Pi to run "headless" (without a monitor) so it doesn't waste resources on a desktop interface.

**1. Connecting to the Network**
We used a mobile hotspot for the network connection. To find your Pi's IP address once it connects to your hotspot:

  * **On Mac/Linux:** Use arp -a or check your hotspot's connected devices list.
  * **On Windows:** Use arp -a in Command Prompt or a tool like Advanced IP Scanner.

**2. Setting up SSH Access (Passwordless Login)**
To make running experiments faster, we set up SSH keys. This allows you to log in without typing your password every time.

**On your laptop (not the Pi):**

**1. Generate a key pair:**

```bash
ssh-keygen -t rsa -b 4096
```
(Give it a descriptive name like ~/.ssh/rpi_key when prompted).

**2. Copy the key to the Pi:**

```bash
ssh-copy-id -i ~/.ssh/rpi_key.pub <username>@<pi_ip_address>
```
**3. Simplify the connection:**
Create or edit the config file on your laptop (nano ~/.ssh/config) and add:

```
Host rpi
    HostName <pi_ip_address>
    User <username>
    IdentityFile ~/.ssh/rpi_key
```
Now you can simply type ssh rpi to log in.

**4. Critical Libraries and Toolchain**
Because some networks (like hotspots) can be unstable for large file transfers, we build all necessary tools directly on the Pi.

  * **WiringPi:** Essential for hardware signal handling. We use the community-maintained version because the original is deprecated.

```bash
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi && ./build
```
  * **Lingua Franca (lfc):** We install the compiler locally to avoid "cross-compilation" issues over the network.

```bash
git clone https://github.com/lf-lang/lingua-franca.git
cd lingua-franca && ./gradlew assemble
```
*Ensure you add lingua-franca/bin to your system PATH so you can run lfc from any directory.*

### STM32 External Signal Generator Wiring
Unlike the original tutorial, which needed a special signal to start the generator, our STM32 starts working automatically the moment it is powered on. This makes the setup much easier because we only need three jumper cables instead of four:

| STM32 Nucleo Pin | RPi 3B+ Physical Pin | RPi Native (BCM) | WiringPi (wPi) | Interrupt Type |
| :--- | :--- | :--- | :--- | :--- |
| **GND** | **Pin 6** | Ground | Ground | Common Ground |
| **D2** | **Pin 10** | BCM 15 | wPi 16 | **Periodic** (10ms intervals) |
| **D3** | **Pin 12** | BCM 18 | wPi 1 | **Sporadic** (Randomized bursts) |

*Note: Ensure the STM32 is powered via USB before initiating the software benchmarks on the Raspberry Pi. The external signal generator code (`metronome.ino`) will immediately begin generating pulses upon boot.*

---

## Software and Platform Setup

The software environment on the Raspberry Pi requires the Lingua Franca compiler, native C build tools, and the WiringPi library to interface with the GPIO pins.

### 1. Network and Base Dependencies
We access the Raspberry Pi through a terminal (SSH). *Note: If you are using a mobile hotspot, it might block you from sending files from your laptop to the Pi. To fix this, we install all the necessary tools and build the programs directly on the Pi instead of trying to move them over the network.*

Update the system and install the required build tools and Python libraries (used for the configuration script):
```bash
sudo apt update
sudo apt install cmake git python3-pip
pip3 install numpy matplotlib
```

### 2. WiringPi Installation
The C program and Lingua Franca rely on WiringPi to handle the hardware signals. Because the original version of WiringPi is no longer being updated, you must install this community-maintained version instead:

```bash
git clone [https://github.com/WiringPi/WiringPi.git](https://github.com/WiringPi/WiringPi.git)
cd WiringPi
./build
```
Verify the installation and your pin mappings by running gpio readall.

### 3. Local Lingua Franca Installation
Instead of building the programs on a laptop and trying to send them over the network, we install the Lingua Franca compiler (lfc) directly on the Raspberry Pi. This avoids any connection issues caused by Wi-Fi hotspots and keeps everything on one device.

Download and install the LF compiler. Make sure to use the version that works with the research library, which is usually version 0.4.1 or the most recent 'development' version mentioned in the tutorial:

### 4. Clone the LF repository and build locally
```bash
git clone [https://github.com/lf-lang/lingua-franca.git](https://github.com/lf-lang/lingua-franca.git)
cd lingua-franca
./gradlew assemble
```
Ensure the ./bin/lfc executable is added to your system $PATH.

# The Benchmarks
We use these four benchmarks to isolate what causes timing delays. We start with basic Linux background noise, then look at the extra work Lingua Franca does, then move to delays caused by multiple processors working at once, and finally test a way to fix those delays automatically.

### 1. Interrupt Robustness in C (src/c/main.c)
This program is our starting point. It sets up the Raspberry Pi pins and waits for a 'start' signal from the STM32. Once it starts, it runs a heavy math loop that keeps the processor busy. By recording the timing of this loop, we can measure the normal delays caused by the Linux operating system as it switches between different background tasks.

### 2. Basic Single-Core Test (src/lf/RPi_Work.lf)
This is our first test using the Lingua Franca language. We run it on a single core and make it do the exact same math as our original C program. By comparing the two, we can see how much extra processing time (overhead) Lingua Franca adds to the system compared to plain C.

### 3. Parallel Reactions in LF (src/lf/RPi_Parallel.lf)
This test pushes the Raspberry Pi to its limit by running the math loop on all four processors at the same time. Because the Pi's processors have to share the same small part of fast memory (the L2 cache), they end up getting in each other's way. We use this test to measure the timing delays caused by this memory interference, proving how difficult it is to keep perfect timing when multiple processors are working in parallel.

### 4. Tight Control Loop in LF (src/lf/RPi_Control.lf)
This benchmark tests a software-based solution for timing delays. By using a feedback loop, the code can sense when a timing spike occurs and react in real-time to correct it. This allows the system to remain consistent and predictable, even when the Raspberry Pi is under heavy pressure from external signals.

# Execution and Visualization
Follow these steps to compile the benchmarks on the Raspberry Pi, capture the timing data, and generate the visualization graphs.

### 1. Generate Configuration
First, determine how many iterations the benchmarks should run. The normal.py script generates a header file (/tmp/config.h) that sets CONFIG_NITERATIONS for the C compiler.

```bash
python3 scripts/normal.py 10000
```
This example configures the system to run 10,000 computational loops.

### 2. Start the External Signal Generator
Ensure the STM32 Nucleo is powered via USB and connected to the Raspberry Pi GPIO pins as detailed in the Hardware Setup. It will immediately begin flooding the Pi with periodic and sporadic interrupts.

### 3. Compile and Run the Benchmarks
All tests should be run one after another, and the results (the timing delays) should be saved directly into text files for analysis. You can do this by using the > command to send the output to the ```data/``` folder.

Standard C starting test:

```bash
gcc src/c/main.c src/c/common.c -o bin/c_main -lwiringPi
./bin/c_main > data/c_baseline_results.txt
```
Lingua Franca Benchmarks:
Use the local LF compiler to build the single-core, parallel, and tight control loop benchmarks. The compiler automatically places the executables in the bin/ folder.

```bash
# Open-Loop Single Core
lfc src/lf/RPi_Work.lf
./bin/RPi_Work > data/baseline_results.txt

# Open-Loop Multi-Core (4 Workers)
lfc src/lf/RPi_Parallel.lf
./bin/RPi_Parallel > data/parallel_results.txt

# Closed-Loop Autonomic Control
lfc src/lf/RPi_Control.lf
./bin/RPi_Control > data/control_results.txt
```
### 4. Data Visualization
Once the hardware execution is complete, the data files are analyzed using Python to generate visual proof of the timing differences between a single-core and a multi-core setup and the LF determinism.

If working remotely, transfer the data/ directory to your local host machine. Run the provided Python graphing scripts.

```bash
python plot_benchmarks.py
```
This script parses the nanosecond timestamps, converts them to milliseconds, and generates the high-resolution visualization graphs found in docs/figs/:

Figure 2. Single core predictability
<img width="3000" height="1500" alt="single_core_predictability" src="https://github.com/user-attachments/assets/f9fdf2d5-89fc-425c-9d9b-8ddc66258e00" />
This graph compares three different single-processor setups. It shows how the "Smart" Feedback Loop (Tight Control Loop) actually improves timing predictability compared to the basic Lingua Franca and standard C versions

Figure 3. Multi-core determinism
<img width="3000" height="1500" alt="multi_core_determinism" src="https://github.com/user-attachments/assets/572421f9-eb4e-4e3a-b1d8-e217024f8d29" />
Demonstrates the timing delays spikes by comparing the single-core C baseline against the 4-core parallel LF execution.
