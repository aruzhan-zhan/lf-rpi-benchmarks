# lf-rpi-benchmarks: Evaluating Lingua Franca Timing Predictability on Raspberry Pi

## Introduction
This repository contains the code and tests used to measure timing predictability and cache contention on a multi-core Raspberry Pi 4B running Linux.

These experiments measure how much a system's timing gets delayed (jitter) when it is constantly bombarded by external hardware interrupts. We compare a standard C program against Lingua Franca to see exactly how much delay is caused when the Raspberry Pi's four physical cores are forced to fight over the same shared memory cache.

### Credits and Departures from Original Work
These benchmarks build directly upon the foundational methodology and library files established in the [programming-pret-machines](https://github.com/magnmaeh/programming-pret-machines) repository. 

While the original tutorial spanned multiple platforms and used a Digilent Analog Discovery tool for waveform generation, this project specifically isolates the **Raspberry Pi 4B** to study multi-core cache behavior. Key departures from the original work include:
1. **External Signal Generator:** Replacing the expensive Digilent Analog Discovery tool with an **STM32 Nucleo-F302R8** programmed via the Arduino IDE to physically generate strict periodic and sporadic hardware interrupts.
2. **Multi-Core Parallelism:** Introducing a 4-core parallel Lingua Franca benchmark (RPi_Parallel.lf) specifically for the Raspberry Pi. This measures the delays caused when multiple processors fight over shared memory, a test that was absent from the original tutorial.

### ⚠️ Important Dependency Note
Because this project utilizes the core library components of the original research, the Lingua Franca files in this repository rely on relative paths (e.g., `../../lib/RPi/interrupt.cmake`). To compile and run these benchmarks, you **must** clone this repository inside the `experiments/lf/src/` directory of the original `programming-pret-machines` repository.

---

## Repository Structure and File Directory

* **`src/c/main.c`**: The standard C program used as our starting point for comparison. It sets up the pins, waits for the hardware signal, runs the math loop, records the timestamps, and prints the timing delays.
* **`src/lf/RPi_Work.lf`**: This runs the exact same test as the standard C program, but using Lingua Franca on a single core. Because Lingua Franca has to do extra work in the background to manage its strict timing rules, this test lets us measure exactly how much extra delay (overhead) the language itself adds.
* **`src/lf/RPi_Parallel.lf`**: This test runs Lingua Franca across all 4 cores of the Raspberry Pi at the same time. Because all 4 processors have to share the same small 'shelf' of fast memory (the L2 cache), they end up fighting for access. We use this test to measure the timing delays caused when the processors get in each other's way.
* **`src/metronome/metronome.ino`**: The C++ firmware flashed to the STM32 Nucleo. It acts as the physical interrupt generator, sending 10ms periodic pulses on one pin and randomized sporadic bursts on another.
* **`scripts/normal.py`**: A small Python script that sets up the test. It creates a configuration file (/tmp/config.h) that tells the standard C program exactly how many times to repeat the math loop so we can get a consistent starting measurement.
* **`data/`**: Contains the raw nanosecond timestamp outputs (`*_results.txt`) from the execution of the four benchmarks.
* **`docs/figs/`**: This folder contains the charts and graphs created by the Python script. These images show the timing differences between a single processor and four processors working at the same time.

---

## Hardware Setup

The physical architecture of this benchmark consists of two primary components: the **Raspberry Pi 4B** (the target platform) and the **STM32 Nucleo-F303R8** (external signal generator).

### Raspberry Pi 4B Configuration
We access the Raspberry Pi remotely through a terminal (SSH). This allows us to run the tests without a desktop interface or a monitor, ensuring the Pi doesn't waste any processing power on unnecessary background tasks.

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
pip3 install numpy matplotlib```

***
### 2. WiringPi Installation
The C baseline and Lingua Franca C-target rely on WiringPi to handle the hardware interrupts. Because the original WiringPi project was deprecated, you must install it from the community-maintained GitHub mirror:

```bash
git clone [https://github.com/WiringPi/WiringPi.git](https://github.com/WiringPi/WiringPi.git)
cd WiringPi
./build
***
Verify the installation and your pin mappings by running gpio readall.

3. Local Lingua Franca Installation
Unlike standard LF cross-compilation workflows, we install the Lingua Franca compiler (lfc) directly onto the Raspberry Pi to avoid network transfer issues.

Download and install the LF compiler (ensure you are using the version compatible with the programming-pret-machines library, typically v0.4.1 or the nightly build specified in the original tutorial):

Bash
# Clone the LF repository and build locally
git clone [https://github.com/lf-lang/lingua-franca.git](https://github.com/lf-lang/lingua-franca.git)
cd lingua-franca
./gradlew assemble
Ensure the ./bin/lfc executable is added to your system $PATH.

The Benchmarks
This research suite consists of four distinct benchmarks. Each step is designed to isolate a specific variable: native execution noise, software framework overhead, multi-core cache contention, and algorithmic timing mitigation.

1. Native C Single-Core Baseline (src/c/main.c)
This bare-metal C program represents the standard execution environment. It configures the GPIO pins using WiringPi, waits for a synchronization signal, and executes a computationally heavy loop for CONFIG_NITERATIONS. It directly handles the hardware interrupts generated by the STM32 metronome. The timestamps recorded here establish the baseline execution jitter caused by Linux OS scheduling and basic CPU context switching.

2. Lingua Franca Open-Loop Baseline (src/lf/RPi_Work.lf)
This benchmark introduces the Lingua Franca runtime. Compiled with single-threaded: true, it executes the exact same computational workload as the Native C baseline using the imported WorkCore reactor. By comparing this dataset against the C baseline, we can measure the inherent overhead and scheduling jitter introduced by LF's software-level timing semantics on a single core.

3. Lingua Franca Multi-Core (src/lf/RPi_Parallel.lf)
This benchmark scales the Lingua Franca execution across the Raspberry Pi's hardware. By declaring workers: 4 in the target properties, the LF runtime utilizes a multi-threaded pool to distribute the WorkCore execution. Because the Raspberry Pi 3B+ features a unified L2 cache shared among its 4 physical cores, this parallel execution explicitly triggers cache contention. The resulting jitter isolates the timing anomalies caused by shared memory architecture, proving the necessity of directory cache coherence.

4. Lingua Franca Closed-Loop Control (src/lf/RPi_Control.lf)
This final benchmark introduces algorithmic mitigation. Also running on Lingua Franca, it replaces the standard work reactor with ControlCoreDelayed. This implements a feedback control loop that actively monitors execution latencies. When latency spikes occur due to interrupts or OS noise, the control loop dynamically reacts to correct the timing variation, establishing a predictable, deterministic execution envelope purely in software.

Execution & Visualization
Follow these steps to compile the benchmarks on the Raspberry Pi, capture the timing data, and generate the visualization graphs.

1. Generate Configuration
First, determine how many iterations the benchmarks should run. The normal.py script generates a header file (/tmp/config.h) that sets CONFIG_NITERATIONS for the C compiler.

Bash
python3 scripts/normal.py 10000
This example configures the system to run 10,000 computational loops.

2. Start the Hardware Metronome
Ensure the STM32 Nucleo is powered via USB and connected to the Raspberry Pi GPIO pins as detailed in the Hardware Setup. It will immediately begin flooding the Pi with periodic and sporadic interrupts.

3. Compile and Run the Benchmarks
All benchmarks must be run sequentially, and their standard output (the execution latencies) should be piped into text files for analysis. Create a data/ directory if you haven't already.

Native C Baseline:

Bash
gcc src/c/main.c src/c/common.c -o bin/c_main -lwiringPi
./bin/c_main > data/c_baseline_results.txt
Lingua Franca Benchmarks:
Use the local LF compiler to build the single-core, parallel, and closed-loop benchmarks. The compiler automatically places the executables in the bin/ folder.

Bash
# Open-Loop Single Core
lfc src/lf/RPi_Work.lf
./bin/RPi_Work > data/baseline_results.txt

# Open-Loop Multi-Core (4 Workers)
lfc src/lf/RPi_Parallel.lf
./bin/RPi_Parallel > data/parallel_results.txt

# Closed-Loop Autonomic Control
lfc src/lf/RPi_Control.lf
./bin/RPi_Control > data/control_results.txt
4. Data Visualization
Once the hardware execution is complete, the data files are analyzed using Python to generate visual proof of the cache contention and the LF determinism.

If working remotely, transfer the data/ directory to your local host machine. Run the provided Python graphing script:

Bash
python plot_benchmarks.py
This script parses the nanosecond timestamps, converts them to milliseconds, and generates the high-resolution visualization graphs found in docs/figs/:

single_core_predictability.png: Compares Native C, LF Open-Loop, and LF Closed-Loop.

multi_core_determinism.png: Demonstrates the cache contention spikes by comparing the single-core C baseline against the 4-core parallel LF execution.
