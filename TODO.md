# TODO

* Storage types for the cartridgeSize value in cartridge.h/cartridge.c - make these consistent and relevant to the actual maximum possible cartridge size - perhaps unsigned aswell.

* Add to/update initialisation routine

* CPU batch run wrapper - include the time taken to execute the actual ops in the calculation of sleep time

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

## Coding Issues

* Const correctness?

* Explanatory comments

* Include guards

## Questions

* RETI instruction - is there a single instruction delay before interrupts are actually enabled as with EI and DI?

## Things of General Interest

* Plot a graph of the average ns delay achieved per single clock cycle and the % CPU utilisation while varying the CPU_MIN_CYCLES_PER_SET parameter