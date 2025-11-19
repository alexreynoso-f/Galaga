# Galaga

A Galaga game project with integrated R data analysis capabilities.

## Project Structure

```
Galaga/
├── main.cpp              # Main game source code
├── CMakeLists.txt        # CMake build configuration
├── data/                 # Game data files
│   └── game_scores.csv   # Sample game performance data
├── analysis/             # R analysis scripts
│   ├── game_analysis.R   # Main analysis script
│   └── README.md         # Analysis documentation
└── plots/                # Generated visualization plots (created by analysis)
```

## Building the Game

### Prerequisites
- CMake 3.10 or higher
- C++ compiler with C++23 support

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

### Run the Game

```bash
./build/Galaga
```

## Data Analysis

This project includes R scripts to analyze game performance data. See the [analysis/README.md](analysis/README.md) for detailed instructions.

### Quick Start for Analysis

```bash
cd analysis
Rscript game_analysis.R
```

This will generate statistical analysis and visualization plots based on the game data.

## Features

- **Game**: Classic Galaga gameplay (in development)
- **Data Tracking**: Record player scores, levels, and performance metrics
- **R Analysis**: Comprehensive statistical analysis of game data
- **Visualizations**: Automatic generation of performance charts and graphs

## Data Collection

Game sessions are recorded in CSV format with the following metrics:
- Player score
- Level reached
- Enemies destroyed
- Time played
- Date of play

## Contributing

Contributions are welcome! Please feel free to submit pull requests.

## License

This project is open source and available for educational purposes.
