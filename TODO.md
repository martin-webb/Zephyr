# TODO

* Divider
	* How is this affected by the CGB double-speed mode?

* Timer
	* How is this affected by the CGB double-speed mode?

* Video
	* Variable length LCDC Mode 3

* Input

* Interrupts
	* V-Blank
	* LCDC Status
	* Timer Overflow
	* Serial Transfer Completion
	* Hi-Lo of P10-P13

* Storage types for the cartridgeSize value in cartridge.h/cartridge.c - make these consistent and relevant to the actual maximum possible cartridge size - perhaps unsigned aswell.

* Merge the GBType and CGBMode enums? Ensure that the types for CGBMode variables are restricted.

* Add to/update initialisation routine

* CPU batch run wrapper - include the time taken to execute the actual ops in the calculation of sleep time

* Consider how the CPU core will be switched to GBP/SGB/GBC mode

* Sound

* Serial I/O

* Battery-backed RAM

* Low-power mode (HALT instruction effects)

* Stop Mode (STOP instruction effects)

* STOP instruction coded as 10 00

* Instruction Skipping After HALT When Interrupts Are Disabled (GB, GBP and SGB but not GBC, even in GB mode ($143=$00))
* Prevent reads or writes to specific I/O registers in the $FF00-$FF4C areas?

* Sprite RAM Bug

* Power Up Sequence - Does the GBP init A to a different value than other GB hardware as the GB CPU Manual PDF suggests?

* Startup Options
	* Game Boy type
		* GB
		* GBP
		* CGB
		* SGB

* Logging options - potentially only in a debug release
	* Log instructions
	* Log interrupts
	* Log I/O register reads/writes
	* Log execution of instruction groups including cycles executed, opcodes executed and sleep
	* Consider using a structured log format such as JSON

* I/O Register Read/Write Correctness
	* IME is Write Only

* Move responsibility for handling the read/write-ability of I/O registers into the components they belong to, instead of the memory controller

* Scanline Rendering: Should I consider some kind of tile data caching system? i.e. read all tile data locally, and if it doesn't change between pixel, don't re-read? With the current system of always drawing as many possible pixels for each tile line data read, this seems unlikely

## Coding Issues

* Const correctness?

* Explanatory comments

* GameBoyType assignment - there is currently no set of conditions that can "detect" a GBP (because this is a hardware function not a feature of the cartridge, however in the emulator the line between the two is slightly blurred - consider adding a "Force GBP" mode so games that look for GBP (by checking the A register) can run "correctly")

## Questions

* RETI instruction - is there a single instruction delay before interrupts are actually enabled as with EI and DI?

* When is LY/FF44 updated? When the new scanline is being rendered (at the beginning of mode 2 (unless in VBLANK)) or when the data is being transferred to the LCD (in mode 3 (unless in VBLANK))?

* What happens in a write to LY? The value of the register is set to 0, but does this reset the rendering timings of the display? That is, does the LCD controller revert to mode 2 (the start of a scanline render) and have an (in my implementation) internal clock cycle count of 0?

* What happens when the LCD is disabled then re-enabled? What mode is it in? What interrupts are triggered?

* LCDC Status interrupt is not generated for the very first frame drawn because we use a separate old and new value of LY (i.e. what is the new value of LY for this update? Is it the same as the old one? Store new value as old value.) to ensure that an LCDC Status interrupt for LY=LYC is only generated once per coincidence, and for the very first run, unless the stored value of LY is set to a non-zero value, then the extra comparison that old LY != new LY will never be true. Considering that the LY=LYC interrupt has to be enabled by the game first, and the LCDC is on by default, this may not be an issue because the GB will never actually need to generate an LCDC Status interrupt for the very first LY=LYC event (assuming both are 0 on startup).

## Things of General Interest

* Plot a graph of the average ns delay achieved per single clock cycle and the % CPU utilisation while varying the CPU_MIN_CYCLES_PER_SET parameter