# Galaga Game Data Analysis

This directory contains R scripts for analyzing Galaga game performance data.

## Prerequisites

- R (version 3.6 or higher)
- Required R packages: `ggplot2`

### Installing R

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install r-base
```

#### macOS
```bash
brew install r
```

#### Windows
Download and install from [CRAN](https://cran.r-project.org/bin/windows/base/)

## Running the Analysis

1. Make sure you have game data in the `../data/game_scores.csv` file

2. Navigate to the analysis directory:
```bash
cd analysis
```

3. Run the analysis script:
```bash
Rscript game_analysis.R
```

The script will automatically install the required `ggplot2` package if it's not already installed.

## Output

The analysis script generates:

### Console Output
- Dataset summary (number of games, date range)
- Score statistics (average, median, max, min, standard deviation)
- Level statistics (average level reached, max level, most common level)
- Enemies destroyed statistics
- Time played statistics
- Performance metrics (score per minute, enemies per minute)
- Correlation analysis

### Visualizations
The script creates a `plots/` directory with the following visualizations:
- `score_distribution.png` - Histogram of score distribution
- `score_vs_level.png` - Scatter plot showing correlation between score and level
- `performance_over_time.png` - Line chart showing score progression over time
- `enemies_by_level.png` - Box plot showing enemies destroyed at each level

## Data Format

The game data should be in CSV format with the following columns:
- `player_id` - Unique identifier for each game session
- `score` - Final score achieved
- `level_reached` - Highest level reached in the game
- `enemies_destroyed` - Total number of enemies destroyed
- `time_played_seconds` - Duration of the game session in seconds
- `date` - Date when the game was played (YYYY-MM-DD format)

## Example

```bash
cd /home/runner/work/Galaga/Galaga/analysis
Rscript game_analysis.R
```

This will analyze the sample data and generate statistical summaries and visualizations.
