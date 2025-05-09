#!/usr/bin/env python3
import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
import glob
import re

def main():
    # Check if dataset name is provided as command line argument
    if len(sys.argv) != 2:
        print("Usage: python3 ShowResult-t.py <dataset_name>")
        sys.exit(1)
    
    dataset_name = sys.argv[1]
    
    # Set font size for all elements in the figure
    matplotlib.rcParams.update({'font.size': 30})
    
    # Find all indexsizelog.csv files in the current directory
    files = glob.glob('a*_indexsizelog.csv')
    
    # Initialize lists to store data
    alphas = []
    delta_indexes = []
    
    # Process each file
    for file in sorted(files):
        try:
            # Extract alpha value from filename (e.g., a05_indexsizelog.csv -> 0.5, a10_indexsizelog.csv -> 1.0)
            # Use regex to extract the numeric part after 'a' and before '_'
            match = re.search(r'a(\d+)_', file)
            if match:
                alpha_str = match.group(1)
                if len(alpha_str) == 2:
                    alpha_value = float(alpha_str) / 10
                else:
                    alpha_value = float(alpha_str) / 100
                
                alphas.append(f'α={alpha_value}')
                
                # Read the CSV file
                df = pd.read_csv(file)
                
                # Get the value of " Deltaindex (%)" from the last row
                delta_index = df[" Deltaindex (%)"].iloc[-1]
                delta_indexes.append(delta_index)
                
                print(f"File: {file}, Alpha: {alpha_value}, Delta Index: {delta_index}")
            else:
                print(f"Could not extract alpha value from filename: {file}")
            
        except Exception as e:
            print(f"Error processing file {file}: {e}")
    
    # Create the bar chart
    plt.figure(figsize=(15, 10))
    
    # Create bars
    bars = plt.bar(alphas, delta_indexes, width=0.6)
    
    # Add labels on top of each bar
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width() / 2, height,
                 f'{abs(height):.3f}', ha='center', va='bottom', fontsize=30)
    
    # Add labels and title
    plt.xlabel(f'Impact of α on delta index size - {dataset_name}', fontsize=30)
    plt.ylabel('Index Overhead (%)', fontsize=30)
    plt.xticks(fontsize=30)
    plt.yticks(fontsize=30)
    
    # Adjust layout
    plt.tight_layout()
    
    # Save the figure
    output_filename = f'impact_alpha_{dataset_name}.png'
    plt.savefig(output_filename)
    print(f"Chart saved as {output_filename}")
    
    # Display the chart
    plt.show()

if __name__ == "__main__":
    main()