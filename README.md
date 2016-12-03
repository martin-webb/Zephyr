# Zephyr

A Nintendo Game Boy and Game Boy Color emulator.

## About

My goal with this project was simply to learn more about the hardware of Nintendo's Game Boy and Game Boy Color consoles while building something cool :)

## Features

* Emulation of the Game Boy and Game Boy Color
* Hardware accurate emulation (an ongoing goal more than a feature), including:
  * CPU: The DI+HALT hardware bug
  * LCD: Inaccessible VRAM and OAM
  * LCD: Limit of 10 sprites per line
  * LCD: Variable length Mode 3
* ROM Only, MBC1, MBC3 and MBC5 cartridge types
* MBC3 RTC support
* Battery saves
* Passes Blargg's CPU instructions and instruction timing tests
* Turbo mode

## Screenshots

### Games

![Emulator Screenshot - The Legend of Zelda: Oracle of Seasons - Screenshot 1](screenshots/Games/TheLegendOfZeldaOracleOfSeasons/Screenshot1.png)
![Emulator Screenshot - The Legend of Zelda: Oracle of Seasons - Screenshot 2](screenshots/Games/TheLegendOfZeldaOracleOfSeasons/Screenshot2.png)
![Emulator Screenshot - The Legend of Zelda: Oracle of Ages - Screenshot 1](screenshots/Games/TheLegendOfZeldaOracleOfAges/Screenshot1.png)
![Emulator Screenshot - The Legend of Zelda: Oracle of Ages - Screenshot 2](screenshots/Games/TheLegendOfZeldaOracleOfAges/Screenshot2.png)
![Emulator Screenshot - The Legend of Zelda: Oracle of Seasons - Screenshot 3](screenshots/Games/TheLegendOfZeldaOracleOfSeasons/Screenshot3.png)
![Emulator Screenshot - The Legend of Zelda: Oracle of Seasons - Screenshot 4](screenshots/Games/TheLegendOfZeldaOracleOfSeasons/Screenshot4.png)
![Emulator Screenshot - The Legend of Zelda: Oracle of Seasons - Screenshot 5](screenshots/Games/TheLegendOfZeldaOracleOfSeasons/Screenshot5.png)
![Emulator Screenshot - The Legend of Zelda: Oracle of Seasons - Screenshot 6](screenshots/Games/TheLegendOfZeldaOracleOfSeasons/Screenshot6.png)
![Emulator Screenshot - The Legend of Zelda: Link's Awakening DX - Screenshot 1](screenshots/Games/TheLegendOfZeldaLinksAwakeningDX/Screenshot1.png)
![Emulator Screenshot - The Legend of Zelda: Link's Awakening DX - Screenshot 2](screenshots/Games/TheLegendOfZeldaLinksAwakeningDX/Screenshot2.png)
![Emulator Screenshot - Pokémon Yellow - Screenshot 1](screenshots/Games/PokemonYellow/Screenshot1.png)
![Emulator Screenshot - Pokémon Yellow - Screenshot 2](screenshots/Games/PokemonYellow/Screenshot2.png)
![Emulator Screenshot - Pokémon Red - Screenshot 1](screenshots/Games/PokemonRed/Screenshot1.png)
![Emulator Screenshot - Pokémon Blue - Screenshot 1](screenshots/Games/PokemonBlue/Screenshot1.png)

### Blargg's Tests

![Emulator Screenshot - Blargg's CPU Instructions Test](screenshots/BlarggsTests/CPUInstructions/Screenshot1.png)
![Emulator Screenshot - Blargg's Instruction Timings Test](screenshots/BlarggsTests/InstructionTiming/Screenshot1.png)

## Roadmap

* Sound support (including output to files containing individual or combined channels)
* Serial data transfer (including TCP/IP game link support for totally rad remote Pokémon battling and trading)
* Save states
* MBC2 cartridge type support
* Emulation accuracy, in order to run *those* games such as Prehistorik Man, Demotronic, etc.
* Graphics upscaling
* VBA-compatible MBC3 RTC save format
* And more to come...
