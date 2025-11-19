#!/usr/bin/env Rscript

# Galaga Game Data Analysis
# This script analyzes game performance data from game_scores.csv

# Load required libraries (install if needed)
if (!require("ggplot2", quietly = TRUE)) {
  install.packages("ggplot2", repos = "http://cran.us.r-project.org")
  library(ggplot2)
}

# Read the game data
cat("Reading game data...\n")
data <- read.csv("../data/game_scores.csv", stringsAsFactors = FALSE)

# Convert date to Date type
data$date <- as.Date(data$date)

# Display basic statistics
cat("\n=== GALAGA GAME STATISTICS ===\n\n")

cat("Dataset Summary:\n")
cat(paste("Total games played:", nrow(data), "\n"))
cat(paste("Date range:", min(data$date), "to", max(data$date), "\n\n"))

# Score statistics
cat("Score Statistics:\n")
cat(paste("  Average Score:", round(mean(data$score), 2), "\n"))
cat(paste("  Median Score:", median(data$score), "\n"))
cat(paste("  Max Score:", max(data$score), "\n"))
cat(paste("  Min Score:", min(data$score), "\n"))
cat(paste("  Standard Deviation:", round(sd(data$score), 2), "\n\n"))

# Level statistics
cat("Level Statistics:\n")
cat(paste("  Average Level Reached:", round(mean(data$level_reached), 2), "\n"))
cat(paste("  Max Level Reached:", max(data$level_reached), "\n"))
cat(paste("  Most Common Level:", names(sort(table(data$level_reached), decreasing = TRUE))[1], "\n\n"))

# Enemies destroyed statistics
cat("Enemies Destroyed Statistics:\n")
cat(paste("  Average Enemies Destroyed:", round(mean(data$enemies_destroyed), 2), "\n"))
cat(paste("  Total Enemies Destroyed:", sum(data$enemies_destroyed), "\n"))
cat(paste("  Max in Single Game:", max(data$enemies_destroyed), "\n\n"))

# Time played statistics
cat("Time Played Statistics:\n")
cat(paste("  Average Time per Game (minutes):", round(mean(data$time_played_seconds) / 60, 2), "\n"))
cat(paste("  Total Time Played (hours):", round(sum(data$time_played_seconds) / 3600, 2), "\n"))
cat(paste("  Longest Game (minutes):", round(max(data$time_played_seconds) / 60, 2), "\n\n"))

# Performance metrics
cat("Performance Metrics:\n")
data$score_per_minute <- data$score / (data$time_played_seconds / 60)
data$enemies_per_minute <- data$enemies_destroyed / (data$time_played_seconds / 60)
cat(paste("  Average Score per Minute:", round(mean(data$score_per_minute), 2), "\n"))
cat(paste("  Average Enemies Destroyed per Minute:", round(mean(data$enemies_per_minute), 2), "\n\n"))

# Correlations
cat("Correlations:\n")
cat(paste("  Score vs Level:", round(cor(data$score, data$level_reached), 3), "\n"))
cat(paste("  Score vs Enemies Destroyed:", round(cor(data$score, data$enemies_destroyed), 3), "\n"))
cat(paste("  Level vs Time Played:", round(cor(data$level_reached, data$time_played_seconds), 3), "\n\n"))

# Create visualizations directory
if (!dir.exists("../plots")) {
  dir.create("../plots")
}

# Generate plots
cat("Generating plots...\n")

# Plot 1: Score distribution
png("../plots/score_distribution.png", width = 800, height = 600)
ggplot(data, aes(x = score)) +
  geom_histogram(bins = 10, fill = "steelblue", color = "black", alpha = 0.7) +
  labs(title = "Galaga Score Distribution", x = "Score", y = "Frequency") +
  theme_minimal() +
  theme(plot.title = element_text(hjust = 0.5, size = 16, face = "bold"))
dev.off()

# Plot 2: Score vs Level
png("../plots/score_vs_level.png", width = 800, height = 600)
ggplot(data, aes(x = level_reached, y = score)) +
  geom_point(color = "darkblue", size = 3, alpha = 0.6) +
  geom_smooth(method = "lm", color = "red", se = TRUE) +
  labs(title = "Score vs Level Reached", x = "Level Reached", y = "Score") +
  theme_minimal() +
  theme(plot.title = element_text(hjust = 0.5, size = 16, face = "bold"))
dev.off()

# Plot 3: Performance over time
png("../plots/performance_over_time.png", width = 800, height = 600)
ggplot(data, aes(x = date, y = score)) +
  geom_line(color = "darkgreen", size = 1) +
  geom_point(color = "darkgreen", size = 2) +
  labs(title = "Score Progression Over Time", x = "Date", y = "Score") +
  theme_minimal() +
  theme(plot.title = element_text(hjust = 0.5, size = 16, face = "bold"))
dev.off()

# Plot 4: Enemies destroyed by level
png("../plots/enemies_by_level.png", width = 800, height = 600)
ggplot(data, aes(x = factor(level_reached), y = enemies_destroyed)) +
  geom_boxplot(fill = "coral", alpha = 0.7) +
  labs(title = "Enemies Destroyed by Level", x = "Level", y = "Enemies Destroyed") +
  theme_minimal() +
  theme(plot.title = element_text(hjust = 0.5, size = 16, face = "bold"))
dev.off()

cat("\nAnalysis complete! Plots saved to ../plots/ directory.\n")
cat("Generated plots:\n")
cat("  - score_distribution.png\n")
cat("  - score_vs_level.png\n")
cat("  - performance_over_time.png\n")
cat("  - enemies_by_level.png\n")
