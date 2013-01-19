# TODO

* Storage types for the cartridgeSize value in cartridge.h/cartridge.c - make these consistent and relevant to the actual maximum possible cartridge size - perhaps unsigned aswell.

* Merge the GBType and CGBMode enums? Ensure that the types for CGBMode variables are restricted.

* Add to/update initialisation routine

* CPU batch run wrapper - include the time taken to execute the actual ops in the calculation of sleep time

* Consider how the CPU core will be switched to GBP/SGB/GBC mode

* Video
* Input
* Interrupts
	* V-Blank
	* LCDC Status
	* Timer Overflow
	* Serial Transfer Completion
	* Hi-Lo of P10-P13
* Timer
* Sound
* Serial I/O

* Low-power mode (HALT instruction effects)
* Stop Mode (STOP instruction effects)

* STOP instruction coded as 10 00

* Instruction Skipping After HALT When Interrupts Are Disabled (GB, GBP and SGB but not GBC, even in GB mode ($143=$00))
* Prevent reads or writes to specific I/O registers in the $FF00-$FF4C areas?

* Sprite RAM Bug

* Power Up Sequence - Does the GBP init A to a different value than other GB hardware as the GB CPU Manual PDF suggests?

## Coding Issues

* Const correctness?

* Explanatory comments

* GameBoyType assignment - there is currently no set of conditions that can "detect" a GBP (because this is a hardware function not a feature of the cartridge, however in the emulator the line between the two is slightly blurred - consider adding a "Force GBP" mode so games that look for GBP (by checking the A register) can run "correctly")

## Questions

* RETI instruction - is there a single instruction delay before interrupts are actually enabled as with EI and DI?

## Things of General Interest

* Plot a graph of the average ns delay achieved per single clock cycle and the % CPU utilisation while varying the CPU_MIN_CYCLES_PER_SET parameter