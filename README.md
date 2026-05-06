# lf-rpi-benchmarks: Timing Predictability in Cyber-Physical Systems

## Introduction
This repository contains the software baseline and benchmarking suite for evaluating timing predictability and cache coherence on a multi-core Raspberry Pi 3B+ running a Linux-based OS. 

The experiments evaluate the execution jitter (time deviation) of a simulated cyber-physical system under heavy external interrupt loads. We compare native bare-metal C execution against the temporal semantics of Lingua Franca (LF), isolating the timing anomalies caused by shared L2 cache contention across multiple cores. 

### Credits and Departures from Original Work
This benchmarking suite builds directly upon the foundational methodology and library files established in the [programming-pret-machines](https://github.com/magnmaeh/programming-pret-machines) repository. 

While the original tutorial spanned multiple platforms and used a Digilent Analog Discovery tool for waveform generation, this project specifically isolates the **Raspberry Pi 3B+** to study multi-core cache behavior. Key departures from the original work include:
1. **Hardware Metronome:** Replacing the expensive Digilent Analog Discovery tool with an **STM32 Nucleo-F303R8** programmed via the Arduino IDE to physically generate strict periodic and sporadic hardware interrupts.
2. **Multi-Core Parallelism:** Introducing a 4-core parallel Lingua Franca benchmark (`RPi_Parallel.lf`) specifically for the Raspberry Pi to explicitly trigger L2 cache contention, which was absent from the original tutorial's RPi setup.

### ⚠️ Important Dependency Note
Because this project utilizes the core library components of the original research, the Lingua Franca files in this repository rely on relative paths (e.g., `../../lib/RPi/interrupt.cmake`). To compile and run these benchmarks, you **must** clone this repository inside the `experiments/lf/src/` directory of the original `programming-pret-machines` repository.

---

## Repository Structure & File Directory

* **`src/c/main.c`**: The native C single-core baseline. It configures the GPIO pins, waits for the hardware trigger, runs the computational loop, logs timestamps using `clock_gettime()`, and prints the execution latency.
* **`src/lf/RPi_Work.lf`**: The Lingua Franca open-loop baseline. It performs the exact same workload as the native C baseline, establishing the overhead of the LF single-threaded runtime.
* **`src/lf/RPi_Parallel.lf`**: The Lingua Franca multi-core benchmark. Configured with `workers: 4`, this file maps the workload across four parallel cores, deliberately stressing the Raspberry Pi's shared L2 cache to observe contention jitter.
* **`src/lf/RPi_Control.lf`**: The Lingua Franca feedback control benchmark (imported from the original tutorial). It implements a closed loop to monitor latency spikes and actuate timing corrections.
* **`src/metronome/metronome.ino`**: The C++ firmware flashed to the STM32 Nucleo. It acts as the physical interrupt generator, sending 10ms periodic pulses on one pin and randomized sporadic bursts on another.
* **`scripts/normal.py`**: A Python utility script. It generates a randomized configuration header (`/tmp/config.h`) specifying the number of iterations for the C baseline to run.
* **`data/`**: Contains the raw nanosecond timestamp outputs (`*_results.txt`) from the execution of the four benchmarks.
* **`docs/figs/`**: Contains the generated `matplotlib` graphs visualizing the single-core predictability and multi-core determinism datasets.

## Hardware Setup

The physical architecture of this benchmark consists of two primary components: the **Raspberry Pi 3B+** (the target platform) and the **STM32 Nucleo-F303R8** (the hardware metronome).

### Raspberry Pi 4B Configuration
The Raspberry Pi serves as the device under test. It is accessed remotely via SSH (over Ethernet or Wi-Fi) to execute the benchmarks without the overhead of a desktop environment or physical peripherals. 

### STM32 Hardware Metronome Wiring
To physically simulate the unpredictable environment of a cyber-physical system, the STM32 Nucleo generates hardware interrupts. These pulses are fed directly into the Raspberry Pi's GPIO pins to compete for the CPU and shared L2 cache.

Unlike the original tutorial which required a dedicated trigger signal to start a Digilent waveform generator, the STM32 is programmed as a continuous, free-running metronome. The wiring is drastically simplified to just three jumper cables:

| STM32 Nucleo Pin | RPi 3B+ Physical Pin | RPi Native (BCM) | WiringPi (wPi) | Interrupt Type |
| :--- | :--- | :--- | :--- | :--- |
| **GND** | **Pin 6** | Ground | Ground | Common Ground |
| **D2** | **Pin 10** | BCM 15 | wPi 16 | **Periodic** (10ms intervals) |
| **D3** | **Pin 12** | BCM 18 | wPi 1 | **Sporadic** (Randomized bursts) |

*Note: Ensure the STM32 is powered via USB before initiating the software benchmarks on the Raspberry Pi. The metronome code (`metronome.ino`) will immediately begin generating pulses upon boot.*

## Software & Platform Setup

The software environment on the Raspberry Pi requires the Lingua Franca compiler, native C build tools, and the WiringPi library to interface with the GPIO pins.

### 1. Network & Base Dependencies
The Raspberry Pi is accessed via SSH. *Note: If you are using a mobile hotspot to connect to the Pi, "Client Isolation" features may block host-to-target protocols like `scp`. To bypass this, we compile everything locally on the Pi rather than transferring binaries from a host machine.*

Update the system and install the required build tools and Python libraries (used for the configuration script):
```bash
sudo apt update
sudo apt install cmake git python3-pip
pip3 install numpy matplotlib

### 2. WiringPi Installation
The C baseline and Lingua Franca C-target rely on WiringPi to handle the hardware interrupts. Because the original WiringPi project was deprecated, you must install it from the community-maintained GitHub mirror:

Bash
git clone [https://github.com/WiringPi/WiringPi.git](https://github.com/WiringPi/WiringPi.git)
cd WiringPi
./build
Verify the installation and your pin mappings by running gpio readall.

### 3. Local Lingua Franca Installation
Unlike standard LF cross-compilation workflows, we install the Lingua Franca compiler (lfc) directly onto the Raspberry Pi to avoid network transfer issues.

Download and install the LF compiler (ensure you are using the version compatible with the programming-pret-machines library, typically v0.4.1 or the nightly build specified in the original tutorial):

Bash
# Clone the LF repository and build locally
git clone [https://github.com/lf-lang/lingua-franca.git](https://github.com/lf-lang/lingua-franca.git)
cd lingua-franca
./gradlew assemble
Ensure the ./bin/lfc executable is added to your system $PATH.

## The Benchmarks

This research suite consists of four distinct benchmarks. Each step is designed to isolate a specific variable: native execution noise, software framework overhead, multi-core cache contention, and algorithmic timing mitigation.

### 1. Native C Single-Core Baseline (`src/c/main.c`)
This bare-metal C program represents the standard execution environment. It configures the GPIO pins using WiringPi, waits for a synchronization signal, and executes a computationally heavy loop for `CONFIG_NITERATIONS`. It directly handles the hardware interrupts generated by the STM32 metronome. The timestamps recorded here establish the baseline execution jitter caused by Linux OS scheduling and basic CPU context switching.

### 2. Lingua Franca Open-Loop Baseline (`src/lf/RPi_Work.lf`)
This benchmark introduces the Lingua Franca runtime. Compiled with `single-threaded: true`, it executes the exact same computational workload as the Native C baseline using the imported `WorkCore` reactor. By comparing this dataset against the C baseline, we can measure the inherent overhead and scheduling jitter introduced by LF's software-level timing semantics on a single core.

### 3. Lingua Franca Multi-Core (`src/lf/RPi_Parallel.lf`)
This benchmark scales the Lingua Franca execution across the Raspberry Pi's hardware. By declaring `workers: 4` in the target properties, the LF runtime utilizes a multi-threaded pool to distribute the `WorkCore` execution. Because the Raspberry Pi 3B+ features a unified L2 cache shared among its 4 physical cores, this parallel execution explicitly triggers cache contention. The resulting jitter isolates the timing anomalies caused by shared memory architecture, proving the necessity of directory cache coherence.

### 4. Lingua Franca Closed-Loop Control (`src/lf/RPi_Control.lf`)
This final benchmark introduces algorithmic mitigation. Also running on Lingua Franca, it replaces the standard work reactor with `ControlCoreDelayed`. This implements a feedback control loop that actively monitors execution latencies. When latency spikes occur due to interrupts or OS noise, the control loop dynamically reacts to correct the timing variation, establishing a predictable, deterministic execution envelope purely in software.

## Execution & Visualization

Follow these steps to compile the benchmarks on the Raspberry Pi, capture the timing data, and generate the visualization graphs.

### 1. Generate Configuration
First, determine how many iterations the benchmarks should run. The `normal.py` script generates a header file (`/tmp/config.h`) that sets `CONFIG_NITERATIONS` for the C compiler.
```bash
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
