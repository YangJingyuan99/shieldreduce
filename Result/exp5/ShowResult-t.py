#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt
import re
import sys
from matplotlib.ticker import AutoMinorLocator

def plot_server_logs(dataset_name):
    # Define file patterns to search for
    file_patterns = ['t00_serverlog.csv', 't0[0-9][0-9]_serverlog.csv', 't01_serverlog.csv']
    
    # Initialize data structures to store our results
    x_values = []
    offline_times = []
    online_times = []
    
    # Process each file
    for pattern in file_patterns:
        for filename in [f for f in os.listdir('.') if re.match(pattern, f)]:
            # Extract x-value from filename
            if filename.startswith('t00_'):
                x_value = 0.0
            elif filename.startswith('t01_'):
                x_value = 0.1
            else:  # t001 to t009
                match = re.search(r't0(\d\d)_', filename)
                if match:
                    x_value = float('0.' + match.group(1))
            
            # Read the last line of the CSV file
            try:
                df = pd.read_csv(filename)
                if not df.empty:
                    last_row = df.iloc[-1]
                    
                    # Extract the required metrics
                    offline_time = last_row[' AverageOfflineTime(s)']
                    online_time = last_row[' AverageOnlineTime(s)']
                    
                    # Store the data
                    x_values.append(x_value)
                    offline_times.append(offline_time)
                    online_times.append(online_time)
                    
                    print(f"Processed {filename}: x={x_value}, offline={offline_time}, inline={online_time}")
                else:
                    print(f"Warning: {filename} is empty")
            except Exception as e:
                print(f"Error processing {filename}: {e}")
    
    # Sort the data by x_values
    sorted_data = sorted(zip(x_values, offline_times, online_times))
    x_values = [item[0] for item in sorted_data]
    offline_times = [item[1] for item in sorted_data]
    online_times = [item[2] for item in sorted_data]
    
    # Create the plot with dual y-axes
    fig, ax1 = plt.subplots(figsize=(12, 8))
    
    # Set color for online time (left y-axis)
    color1 = 'tab:blue'
    ax1.set_xlabel(f'Value (Dataset: {dataset_name})', fontsize=30)
    ax1.set_ylabel('Inline Time (s)', fontsize=30, color=color1)
    line1 = ax1.plot(x_values, online_times, 'o-', color=color1, linewidth=2, 
                     label='Average Inline Time', markersize=10)
    ax1.tick_params(axis='y', labelcolor=color1, labelsize=30)
    ax1.tick_params(axis='x', labelsize=30)
    
    # Create the second y-axis for offline time
    ax2 = ax1.twinx()
    color2 = 'tab:red'
    ax2.set_ylabel('Offline Time (s)', fontsize=30, color=color2)
    line2 = ax2.plot(x_values, offline_times, 's-', color=color2, linewidth=2, 
                     label='Average Offline Time', markersize=10)
    ax2.tick_params(axis='y', labelcolor=color2, labelsize=30)
    
    # Add grid for better readability (only for the primary axis to avoid cluttering)
    ax1.grid(True, linestyle='--', alpha=0.7)
    
    # Combine legends from both axes
    lines = line1 + line2
    labels = [l.get_label() for l in lines]
    ax1.legend(lines, labels, fontsize=30, loc='best')
    
    # Add title
    plt.title(f'Average Inline and Offline Times - {dataset_name}', fontsize=30)
    
    # Adjust layout to make room for the large text
    fig.tight_layout()
    
    # Save the plot
    output_filename = f'impact_t_{dataset_name}.png'
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    print(f"Plot saved as {output_filename}")
    
    # Display the plot
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 ShowResult-t.py <dataset_name>")
        sys.exit(1)
    
    dataset_name = sys.argv[1]
    plot_server_logs(dataset_name)