#================================================================================
# File: plot_csv.py                                                               
# Creator: Claudio Raimondi                                                       
# Email: claudio.raimondi@pm.me                                                   
# 
# created at: 2025-02-28 19:14:47                                                 
# last edited: 2025-02-28 19:14:47                                                
#================================================================================

import argparse
import pandas as pd
import plotly.express as px

def parse_arguments():
  parser = argparse.ArgumentParser(description="Plot throughput from multiple CSV files.")
  parser.add_argument("files", type=str, nargs="+", help="Paths to the CSV files")
  return parser.parse_args()

def load_data(files):
  data = []
  x_label, y_label = None, None
  
  for file in files:
    df = pd.read_csv(file)
    
    if x_label is None and y_label is None:
      x_label, y_label = df.columns[:2]
    elif df.columns[0] != x_label or df.columns[1] != y_label:
      raise ValueError(f"Column names in {file} do not match the first file.")
    
    data.append((df[x_label], df[y_label], file))
  
  return data, x_label, y_label

def create_plot(data, x_label, y_label, output_file="plot.png"):
  fig = px.line(title=f"{y_label} vs. {x_label}")
  
  for x, y, label in data:
    fig.add_scatter(x=x, y=y, mode="lines+markers", name=label)
  
  fig.update_layout(
    xaxis_title=x_label,
    yaxis_title=y_label,
    template="plotly_dark",
    legend=dict(orientation="h", yanchor="bottom", y=-0.3, xanchor="center", x=0.5),
    width=1280,
    height=720
  )
  
  fig.write_image(output_file)
  print(f"Static plot saved as {output_file}")

def main():
  args = parse_arguments()
  data, x_label, y_label = load_data(args.files)
  create_plot(data, x_label, y_label)

if __name__ == "__main__":
  main()