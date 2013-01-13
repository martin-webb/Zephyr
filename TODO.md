# TODO

* Add to initialisation routine

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

## Questions

* RETI instruction - is there a single instruction delay before interrupts are actually enabled as with EI and DI? 