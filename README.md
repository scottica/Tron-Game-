# FPGA TRON Game

## 🎮 Overview
A real-time TRON-style light-cycle game written in bare-metal C for the Nios V (RISC-V) soft processor on the DE10-Lite / DE1-SoC. The player races a bot opponent, with timer interrupts driving the game tick, pushbutton interrupts handling steering, and graphics drawn directly to the VGA pixel buffer. Built for a UBC computer systems course on top of the course's interrupt framework.

## ✨ Key Features
* **VGA Graphics:** Draws the arena, light trails, and win screens by writing 16-bit RGB565 pixels directly to the memory-mapped VGA pixel buffer.
* **Interrupt-Driven Steering:** Pushbutton interrupts queue a left or right turn, which is applied on the next game tick. Pressing the same button twice cancels the queued turn.
* **Bot Opponent:** Checks the pixel ahead each tick and turns left, or right if left is also blocked, to avoid walls and trails.
* **Adjustable Speed:** Six switches each double the game speed, for up to 64x, by shortening the machine-timer period.
* **Scoreboard:** Scores appear on the 7-segment displays, and the first player to 9 points wins.

## ⚙️ Technical Implementation
* **Language:** C with the course-provided inline RISC-V CSR setup
* **Hardware:** Intel DE10-Lite / DE1-SoC (Nios V), VGA output
* **Game Tick:** The RISC-V machine timer (`mtimecmp`) fires periodic interrupts, and the handler reads `mcause` to separate timer ticks from pushbutton interrupts.
* **Memory-Mapped I/O:** LEDs, switches, keys, HEX displays (`0xFF200000` range), and the VGA pixel buffer (`0x08000000`).

## 🚀 How to Play
1. The player (blue) and bot (red) start on opposite sides of the arena.
2. Use the pushbuttons to turn left or right relative to your current direction.
3. Flip switches to speed the game up.
4. Avoid walls and trails. Each crash awards a point to the other side, and the first to 9 wins.

## 🔧 Possible Improvements
Moving screen redraws and printing out of the timer interrupt into the main loop would keep interrupt handlers short.
