# FPGA TRON Game

## 🎮 Overview
A real-time, TRON-style light cycle game built for the DE10-Lite / DE1-SoC FPGA boards. Written in C with inline RISC-V assembly, this project bypasses standard operating systems to interact directly with the hardware. It features an interrupt-driven architecture to manage game state, player inputs, and VGA graphics rendering.

## ✨ Key Features
* **Real-Time VGA Graphics:** Renders the game board, player trails, and collision events by writing pixels directly to the FPGA's VGA video memory using precise pointer arithmetic.
* **Interrupt-Driven Controls:** Utilizes hardware pushbutton interrupts for zero-latency, responsive player steering.
* **Automated Bot Opponent:** Features a built-in bot opponent with basic collision avoidance logic.
* **Adjustable Difficulty:** Reads physical hardware switches (`SW`) to dynamically adjust the game speed/clock multiplier in real-time.
* **Hardware Scoreboard:** Tracks points up to a 9-point win condition, outputting the live score directly to the board's 7-segment displays (`HEX`).

## ⚙️ Technical Implementation
* **Languages:** C, RISC-V Assembly (Inline)
* **Hardware:** Intel DE10-Lite / DE1-SoC FPGA
* **Architecture:** * **Machine Timer (`MTIMER`):** Configured via inline RISC-V assembly (`csrr`, `csrc`, `csrs`) to generate periodic interrupts for the main game tick, ensuring deterministic frame updates.
  * **Memory-Mapped I/O:** Extensive use of physical memory addresses (`0xFF200000` range) to interface with LEDs, Switches, Keys, and the VGA Pixel Buffer (`0x08000000`).
  * **Interrupt Service Routines (ISRs):** Custom trap handlers evaluate `mcause` registers to branch between timer ticks and asynchronous key presses, maintaining smooth gameplay without polling delays.

## 🚀 How to Play
1. The Player (Blue) and Bot (Red) spawn on the grid.
2. Use the pushbuttons to steer your light cycle (Left/Right relative to current direction).
3. Use the hardware switches to multiply the game speed. 
4. Avoid walls and light trails! Forcing a crash awards a point. First to 9 points wins the match.
