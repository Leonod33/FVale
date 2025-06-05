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
- Multiple NPCs with interactive dialogue
- Improved parser for commands like `talk to hermit`
- Rooms remember if you've visited them before
- Dynamic weather events that shift as you explore
- Simple quest system with item crafting and puzzles

## Controls / Commands
- `look` / `examine [item]` — View surroundings or inspect inventory items
- `go [direction]` — Move between rooms (north, south, east, west)
- `take [item]` — Pick up an item from the current room
- `combine [item1] [item2]` — Craft a new item from two others
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
- NPC interactions
- Quests
- Puzzles/Riddles
- Specific item uses (Key for a locked door, herbs for healing, lump of iron for a quest etc.)
- Save/Load system

---

Happy wandering!
