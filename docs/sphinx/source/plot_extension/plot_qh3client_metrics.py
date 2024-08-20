import os
import pandas as pd
import plotly.graph_objs as go
import plotly.io as pio
from sphinx.util import logging
from datetime import datetime

logger = logging.getLogger(__name__)

def generate_plot(app):
    # Directory containing CSV files
    csv_dir = os.path.join(app.srcdir, '../../../qfist/')
    
    # Find all summary CSV files
    files = [f for f in os.listdir(csv_dir) if f.startswith('summary-') and f.endswith('.csv')]

    # Extract timestamp from file name and sort by it
    def extract_timestamp(file_name):
        timestamp_str = file_name[len('summary-'):-len('.csv')]  # Extract the timestamp part
        return datetime.strptime(timestamp_str, '%Y-%m-%d_%H-%M-%S')  # Convert to datetime object

    # Sort files by extracted timestamps in reverse order (most recent first)
    files.sort(key=extract_timestamp, reverse=True)

    # Get the most recent 5 files
    recent_files = files[:5]

    if not recent_files:
        logger.warning("No summary CSV files found.")
        return

    # Prepare the combined HTML content
    combined_html_content = ""

    # Generate a plot for each recent file
    for file in recent_files:
        csv_path = os.path.join(csv_dir, file)
        
        try:
            # Read the CSV file
            df = pd.read_csv(csv_path)
        except Exception as e:
            logger.error(f"Error reading {csv_path}: {e}")
            continue

        # Prepare the plot
        fig = go.Figure()

        # Plot Success Percentage against Total Requests
        fig.add_trace(go.Scatter(
            x=df['Total Requests'], y=df['Success Percentage'], mode='lines+markers',
            name='Success Percentage', line=dict(color='blue'), yaxis='y1'
        ))

        # Plot Avg Response Time (ms) against Total Requests
        fig.add_trace(go.Scatter(
            x=df['Total Requests'], y=df['Avg Response Time (ms)'], mode='lines+markers',
            name='Avg Response Time (ms)', line=dict(color='green'), yaxis='y2'
        ))

        # Plot Data Transfer Rate (KB/s) against Total Requests
        fig.add_trace(go.Scatter(
            x=df['Total Requests'], y=df['Data Transfer Rate (KB/s)'], mode='lines+markers',
            name='Data Transfer Rate (KB/s)', line=dict(color='red'), yaxis='y3'
        ))

        # Plot Requests per seconds against Total Requests
        fig.add_trace(go.Scatter(
            x=df['Total Requests'], y=df['Rq/s'], mode='lines+markers',
            name='Rq/s', line=dict(color='orange'), yaxis='y4'
        ))

        # Customize layout with properly configured y-axes
        fig.update_layout(
            title=f'Performance Metrics - {file}',
            xaxis_title='Total Requests',
            yaxis=dict(
                title='Success Percentage',
                titlefont=dict(color='blue'),
                tickfont=dict(color='blue'),
                showgrid=False
            ),
            yaxis2=dict(
                title='Avg Response Time (ms)',
                titlefont=dict(color='green'),
                tickfont=dict(color='green'),
                overlaying='y',
                side='right',
                showgrid=False,
                position=0.85
            ),
            yaxis3=dict(
                title='Data Transfer Rate (KB/s)',
                titlefont=dict(color='red'),
                tickfont=dict(color='red'),
                overlaying='y',
                side='right',
                showgrid=False,
                position=0.75  # Adjusted position for separation
            ),
            yaxis4=dict(
                title='Rq/s',
                titlefont=dict(color='orange'),
                tickfont=dict(color='orange'),
                overlaying='y',
                side='right',
                showgrid=False,
                position=0.65  # Adjusted position for separation
            )
        )

        # Convert plot to HTML string
        plot_html = pio.to_html(fig, include_plotlyjs=False, full_html=False)
        combined_html_content += f"" + plot_html + "<br>"

    # Save the combined HTML file with all plots
    output_html_path = os.path.join(app.outdir, './qh3client_metrics_plot.html')
    
    # Include Plotly.js manually in the head
    plotly_js_cdn = """
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
    """
    
    final_html_content = f"<html><head>{plotly_js_cdn}</head><body>{combined_html_content}</body></html>"

    try:
        # Write the final HTML content to the output file
        with open(output_html_path, 'w') as f:
            f.write(final_html_content)
        logger.info(f"Generated combined plot: {output_html_path}")
    except Exception as e:
        logger.error(f"Error writing HTML file: {e}")

def setup(app):
    app.connect('builder-inited', generate_plot)
