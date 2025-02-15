#================================================================================
# File: plot_csv.py                                                               
# Creator: Claudio Raimondi                                                       
# Email: claudio.raimondi@pm.me                                                   
# 
# created at: 2025-02-15 17:33:44                                                 
# last edited: 2025-02-15 17:33:44                                                
#================================================================================

import pandas as pd
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser(description="Plot throughput from a CSV file.")
parser.add_argument("file", type=str, help="Path to the CSV file")
args = parser.parse_args()

df = pd.read_csv(args.file)

x_label = df.columns[0]
y_label = df.columns[1]

plt.figure(figsize=(8, 5))
plt.plot(df[x_label], df[y_label], marker="o", linestyle="-", color="r")
plt.xlabel(x_label)
plt.ylabel(y_label)
plt.title(f"{y_label} vs. {x_label}")
plt.grid(True)

output_file = args.file.replace(".csv", "_plot.png")
plt.savefig(output_file)

print(f"Plot saved as {output_file}")