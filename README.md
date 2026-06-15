# 🌙 Dark Dave — The Haunted Mansion

A dark horror reimagining of the classic Dangerous Dave game,
built from scratch in C++ with SDL2.

## 🎮 Gameplay
<img width="996" height="780" alt="Screenshot 2026-06-15 153557" src="https://github.com/user-attachments/assets/fb9fe789-e881-4864-8e93-f904b40b4d31" />

Dave receives a mysterious letter inviting him to Blackwood 
Mansion. Upon arrival, the doors lock behind him. Collect 
cursed gems, avoid ghosts, and escape before midnight.

## ✨ Features
- Tile-based level system loaded from text files
- Custom physics engine (gravity, jumping, AABB collision)
- Animated ghost enemies with floating AI
- NPC dialogue system with story elements
- Gothic horror visual theme with real sprite assets
- Lives system with atmospheric death messages
- SDL2_ttf gothic font rendering

## 🛠️ Built With
- C++17
- SDL2 / SDL2_image / SDL2_ttf
- MinGW64 on Windows

## 🚀 How To Build

### Requirements
- MinGW64 g++ compiler
- SDL2 development libraries
- SDL2_image development libraries  
- SDL2_ttf development libraries

### Compile
g++ src/main.cpp -o game.exe

-IC:/SDL2mingw/x86_64-w64-mingw32/include

-LC:/SDL2mingw/x86_64-w64-mingw32/lib

-lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf

###RUN
.\game.exe
