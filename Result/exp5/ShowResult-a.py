import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

def create_bar_chart(dataset_name):
    # List of CSV files
    csv_files = ["a00_serverlog.csv", "a05_serverlog.csv", "a07_serverlog.csv", 
                "a09_serverlog.csv", "a10_serverlog.csv"]

    # Initialize lists to store data
    alpha_values = []
    offline_times = []

    # Process each CSV file
    for file in csv_files:
        try:
            # Extract alpha value from filename (e.g., a07 -> 0.7)
            alpha_str = file[1:3]
            alpha_value = float(alpha_str) / 10
            alpha_values.append(f"α={alpha_value}")
            
            # Read the CSV file
            df = pd.read_csv(file)
            
            # Get the AverageOfflineTime from the last row
            last_row = df.iloc[-1]
            offline_time = last_row[" AverageOfflineTime(s)"]
            offline_times.append(offline_time)
            
            print(f"Extracted from {file}: AverageOfflineTime = {offline_time} seconds")
        except Exception as e:
            print(f"Error processing {file}: {e}")

    # Create the bar chart
    plt.figure(figsize=(12, 8))
    bars = plt.bar(alpha_values, offline_times, color='skyblue', width=0.6)

    # Add labels and title with dataset name
    plt.xlabel(f'Impact of α on offline duration ({dataset_name})', fontsize=30)
    plt.ylabel('Time Duration (s)', fontsize=30)
    plt.title(f'Average Offline Time for Different Alpha Values - {dataset_name}', fontsize=30)

    # Add value labels on top of each bar
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom', fontsize=24)

    # Increase font size for tick labels
    plt.xticks(fontsize=30)
    plt.yticks(fontsize=30)

    # Adjust layout and add grid
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.tight_layout()

    # Save the figure with dataset name in the filename
    output_filename = f'impact_alpha_{dataset_name}.png'
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')

    print(f"Bar chart has been created and saved as '{output_filename}'")

    # Show the plot
    plt.show()

if __name__ == "__main__":
    # Check if dataset name is provided as command line argument
    if len(sys.argv) < 2:
        print("Error: Dataset name is required")
        print("Usage: python3 ShowResult-a.py <dataset_name>")
        sys.exit(1)
    
    # Get dataset name from first command line argument
    dataset_name = sys.argv[1]
    
    # Create the bar chart with the specified dataset name
    create_bar_chart(dataset_name)