# RGBCaro ⚔️⭕❌

A unique tactical twist on the classic Caro (Gomoku) game, developed in C++ using the [Raylib](https://www.raylib.com/) library. RGBCaro combines traditional board game logic with RPG-style combat mechanics!

## ✨ Features
* **Combat Caro:** Getting 5 in a row is only half the battle. Winning a single Caro round allows your character to launch an attack on the opponent.
* **Character-Driven Stats:** The damage dealt and received during an attack is heavily influenced by the unique properties and stats of the chosen characters.
* **HP-Based Matches:** A full match spans across multiple Caro rounds. The game only ends when one player's HP drops to zero. Fight for your survival!
* **Versatile Game Modes:** Duel locally with a friend (PvP) or challenge the built-in Bot AI (PvE).
* **Audio & State Management:** Integrated background music, sound effects, and the ability to save/load your match progress at any time.
* **Clean Architecture:** Codebase is structured firmly upon the MVC (Model-View-Controller) design pattern.


## Build and Execute
### Windows (Visual Studio IDE environment)
Open `RGBCaro.slnx` as a solution with Visual Studio. Build and execute the program within Visual Studio's GUI. NuGet package manager will automatically install the required packages (e.g. `raylib`).

### Linux (Manual build)
**Requirements:** `g++` compiler and `raylib` library installed

**Build**
*(create `out/` if not yet created)*
```bash
mkdir out
```
build command
```bash
g++ src/*.cpp -o ./out/RGBCaro -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```

**Execute**
```bash
./out/RGBCaro
```