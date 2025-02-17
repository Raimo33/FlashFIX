#================================================================================
# File: plot_csv.py                                                               
# Creator: Claudio Raimondi                                                       
# Email: claudio.raimondi@pm.me                                                   
# 
# created at: 2025-02-17 15:08:46                                                 
# last edited: 2025-02-17 15:08:46                                                
#================================================================================

import pandas as pd
import plotly.express as px
import argparse

parser = argparse.ArgumentParser(description="Plot throughput from multiple CSV files.")
parser.add_argument("files", type=str, nargs="+", help="Paths to the CSV files")
args = parser.parse_args()

x_label = None
y_label = None
data_to_plot = []

for file in args.files:
    df = pd.read_csv(file)
    
    if x_label is None and y_label is None:
        x_label = df.columns[0]
        y_label = df.columns[1]
    elif df.columns[0] != x_label or df.columns[1] != y_label:
        raise ValueError(f"Labels in {file} do not match the labels in the first file.")
    
    data_to_plot.append((df[x_label], df[y_label], file))

fig = px.line(title=f"{y_label} vs. {x_label}")

for x, y, label in data_to_plot:
    fig.add_scatter(x=x, y=y, mode="lines+markers", name=label)

fig.update_layout(
    xaxis_title=x_label,
    yaxis_title=y_label,
    template="plotly_dark",
    legend=dict(
        orientation="h",
        yanchor="bottom",
        y=-0.3,
        xanchor="center",
        x=0.5
    ),
    width=1280,
    height=720
)

output_file = "plot.png"
fig.write_image(output_file)

print(f"Static plot saved as {output_file}")