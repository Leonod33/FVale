# Whispers of the Forgotten Vale

A gentle, atmospheric text adventure game in C++.  
Explore forgotten ruins, collect curious items, and uncover the valley’s secrets.

The main game loop lives in `main.cpp`, while room definitions reside in `room.h`
and `room.cpp` for clarity.

## Features
- Explore interconnected rooms
- Fuzzy command recognition and synonyms
- Simple inventory and item usage
- Room descriptions, item examination, and more

## Controls / Commands
- `look` / `examine [item]` — View surroundings or inspect inventory items
- `go [direction]` — Move between rooms (north, south, east, west)
- `take [item]` — Pick up an item from the current room
- `inventory` or `i` — Show carried items
- `help` — List commands
- `exit` — Quit game

## Building & Running
Requires a C++17+ compiler.  
Compile with: g++ main.cpp room.cpp -o vale

Then run: ./vale

## Project Structure
- `main.cpp` – core game loop and logic
- `room.h` – Room structure definition
- `room.cpp` – (currently empty) implementations for any Room methods

## TODO
NPC's
NPC interactions
Quests
Puzzles/Riddles
Specific item uses (Key for a locked door, herbs for healing, lump of iron for a quest etc.)
Dynamic weather
Room visit history
Save/Load system

---

Happy wandering!
