# Knight Hacks IX
## From Point A to Point B
Presented by Kevin Wall and Miguel De La Cruz

Algorithms come in many forms, including finding a path between two points.
This project shows popular pathfinding algorithms and how they react to changes in the map in real time.

It builds with CMake on Windows, macOS and Linux, and downloads wxWidgets 3.2.11 on its own.

### Algorithms
- Breadth-first search
- Dijkstra (uniform cost)
- A* (A-Star)

### Building
You will need CMake 3.28 or newer and a C++20 compiler. On Linux, install the GTK 3 development package first (`sudo apt install libgtk-3-dev` on Ubuntu).
- To install CMake, visit https://cmake.org/download/
- Run a command prompt window pointing to the folder this ReadMe is located in, run the following commands:

```
cmake -B build
cmake --build build --config Release
```

The first build also compiles wxWidgets, which takes a few minutes.
The app ends up in `build/bin` (`build/bin/Release` with Visual Studio).
Visual Studio can also open this folder directly with File > Open > Folder.
Press F1 in the app to see the controls.

### Interesting Map Seeds
The following seeds can yield interesting results for the default 36 x 22 map size.
You may need to modify the heuristic weight or algorithm.
- 6564
- 17420
- 1553
- 12046
- 17050
- 4
- 1234586747
- 1083948475
