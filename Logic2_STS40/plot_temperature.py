"""
STS40 Temperature Plotter
Plots temperature data exported from Saleae Logic2's Data Table

Usage:
    1. In Logic2, open the Data Table for your STS40 analyzer
    2. Click "Export Table" and save as CSV
    3. Run this script: python plot_temperature.py <csv_file>
"""

import sys
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime


def plot_temperature(csv_file):
    """
    Plot temperature data from Logic2 CSV export
    
    Args:
        csv_file: Path to exported CSV file from Logic2
    """
    try:
        # Load CSV data
        print(f"Loading data from {csv_file}...")
        data = pd.read_csv(csv_file)
        
        # Filter only temperature frames
        temp_data = data[data['type'] == 'temperature'].copy()
        
        if temp_data.empty:
            print("No temperature data found in the CSV file!")
            print("Make sure you exported the STS40 analyzer data table.")
            return
        
        print(f"Found {len(temp_data)} temperature readings")
        
        # Convert time to relative seconds (if absolute timestamps)
        if 'time' in temp_data.columns:
            temp_data['time_s'] = temp_data['time']
            # If time is in nanoseconds or very large, normalize it
            if temp_data['time_s'].max() > 3600:  # More than 1 hour in seconds
                temp_data['time_s'] = temp_data['time_s'] - temp_data['time_s'].min()
        else:
            # Create time index
            temp_data['time_s'] = range(len(temp_data))
        
        # Create figure with subplots
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))
        
        # Plot 1: Temperature in Celsius
        ax1.plot(temp_data['time_s'], temp_data['celsius'], 
                marker='o', markersize=4, linestyle='-', linewidth=2, 
                color='#FF6B6B', label='Temperature')
        ax1.set_xlabel('Time (s)', fontsize=12)
        ax1.set_ylabel('Temperature (°C)', fontsize=12)
        ax1.set_title('STS40 Temperature Readings Over Time', fontsize=14, fontweight='bold')
        ax1.grid(True, alpha=0.3, linestyle='--')
        ax1.legend(loc='upper right')
        
        # Add statistics to plot
        mean_temp = temp_data['celsius'].mean()
        min_temp = temp_data['celsius'].min()
        max_temp = temp_data['celsius'].max()
        std_temp = temp_data['celsius'].std()
        
        stats_text = f'Mean: {mean_temp:.2f}°C\nMin: {min_temp:.2f}°C\nMax: {max_temp:.2f}°C\nStd Dev: {std_temp:.2f}°C'
        ax1.text(0.02, 0.98, stats_text, transform=ax1.transAxes, 
                fontsize=10, verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
        
        # Plot 2: Temperature in Fahrenheit
        ax2.plot(temp_data['time_s'], temp_data['fahrenheit'], 
                marker='s', markersize=4, linestyle='-', linewidth=2, 
                color='#4ECDC4', label='Temperature')
        ax2.set_xlabel('Time (s)', fontsize=12)
        ax2.set_ylabel('Temperature (°F)', fontsize=12)
        ax2.set_title('STS40 Temperature Readings (Fahrenheit)', fontsize=14, fontweight='bold')
        ax2.grid(True, alpha=0.3, linestyle='--')
        ax2.legend(loc='upper right')
        
        # Add statistics to plot
        mean_temp_f = temp_data['fahrenheit'].mean()
        min_temp_f = temp_data['fahrenheit'].min()
        max_temp_f = temp_data['fahrenheit'].max()
        std_temp_f = temp_data['fahrenheit'].std()
        
        stats_text_f = f'Mean: {mean_temp_f:.2f}°F\nMin: {min_temp_f:.2f}°F\nMax: {max_temp_f:.2f}°F\nStd Dev: {std_temp_f:.2f}°F'
        ax2.text(0.02, 0.98, stats_text_f, transform=ax2.transAxes, 
                fontsize=10, verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
        
        plt.tight_layout()
        
        # Save figure
        output_file = csv_file.replace('.csv', '_plot.png')
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"\nPlot saved to: {output_file}")
        
        # Show plot
        plt.show()
        
        # Print summary
        print("\n" + "="*60)
        print("TEMPERATURE SUMMARY")
        print("="*60)
        print(f"Total readings:    {len(temp_data)}")
        print(f"Duration:          {temp_data['time_s'].max() - temp_data['time_s'].min():.2f} seconds")
        print(f"\nCelsius:")
        print(f"  Mean:            {mean_temp:.2f}°C")
        print(f"  Min:             {min_temp:.2f}°C")
        print(f"  Max:             {max_temp:.2f}°C")
        print(f"  Range:           {max_temp - min_temp:.2f}°C")
        print(f"  Std Deviation:   {std_temp:.2f}°C")
        print(f"\nFahrenheit:")
        print(f"  Mean:            {mean_temp_f:.2f}°F")
        print(f"  Min:             {min_temp_f:.2f}°F")
        print(f"  Max:             {max_temp_f:.2f}°F")
        print(f"  Range:           {max_temp_f - min_temp_f:.2f}°F")
        print(f"  Std Deviation:   {std_temp_f:.2f}°F")
        print("="*60)
        
    except FileNotFoundError:
        print(f"Error: File '{csv_file}' not found!")
    except KeyError as e:
        print(f"Error: Required column {e} not found in CSV!")
        print("Make sure you exported the STS40 analyzer data table from Logic2.")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_temperature.py <csv_file>")
        print("\nExample:")
        print("  python plot_temperature.py sts40_data.csv")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    plot_temperature(csv_file)
