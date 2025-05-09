import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

def create_bar_chart(dataset_name):
    # Read the CSV file
    try:
        df = pd.read_csv('a00_indexsizelog.csv')
    except FileNotFoundError:
        print("Error: The file a00_indexsizelog.csv was not found in the current directory.")
        return
    
    # Get the last row from the dataframe
    last_row = df.iloc[-1]
    
    # Extract the required values
    try:
        fp_index = float(last_row[" FPindex (%)"])
        delta_index = float(last_row[" Deltaindex (%)"])
        sf_index = float(last_row[" SFindex (%)"])
    except KeyError as e:
        print(f"Error: Could not find one or more required columns. Missing column: {e}")
        return
    
    # Create a bar chart with the specified order
    categories = ['Fingerprint Index', 'Feature Index', 'Delta Index']
    values = [fp_index, sf_index, delta_index]  # Reordering values to match categories
    
    # Set the figure size and font size
    plt.figure(figsize=(12, 8))
    plt.rcParams.update({'font.size': 30})
    
    # Create bars
    bars = plt.bar(categories, values, color=['blue', 'red', 'green'])  # Reordered colors to match the new order
    
    # Add the values above each bar with 3 decimal places
    for bar, value in zip(bars, values):
        plt.text(bar.get_x() + bar.get_width()/2, 
                value * 1.05, 
                f"{value:.3f}", 
                ha='center', 
                fontsize=30)
    
    # Add labels and title
    plt.xlabel(f'The fraction of index size over logical size - {dataset_name}', fontsize=30)
    plt.ylabel('Index Overhead (%)', fontsize=30)
    
    # Set y-axis limit to ensure values are visible
    plt.ylim(0, max(values) * 1.2)
    
    # Add legend
    plt.legend(bars, categories, fontsize=30)
    
    # Adjust layout
    plt.tight_layout()
    
    # Save the figure
    plt.savefig(f'index_overhead_{dataset_name}.png')
    print(f"Bar chart saved as impact_t_{dataset_name}.png")

if __name__ == "__main__":
    # Check if dataset name is provided as an argument
    if len(sys.argv) != 2:
        print("Usage: python3 ./ShowResult-t.py <dataset_name>")
        sys.exit(1)
    
    dataset_name = sys.argv[1]
    create_bar_chart(dataset_name)