import psutil
import time
import requests
import argparse
from datetime import datetime
import socket
import os

# Default values for thresholds and intervals
DEFAULT_CPU_THRESHOLD = 10.0
DEFAULT_CHECK_INTERVAL = 10
DEFAULT_SAMPLE_HISTORY = 4
MAX_TOP_PROCESS_TO_REPORT = 5

# Discord webhook URL (replace with your actual webhook URL)
DISCORD_WEBHOOK_URL = 'https://discord.com/api/webhooks/1312803357462630447/V4KKjgNdx3bCFQ7rNuB3e0Gcjqu1ucWji6Pb2HreuuDgwIyk_ttbbZKJsfSHiNj39seu'

# Reporting limits and cooldowns
REPORT_LIMIT = 10  # Maximum number of reports before cooldown
COOLDOWN_TIME = 45  # Cooldown time in seconds after max reports
MAX_CYCLES = 5  # Maximum number of cycles before stopping

# Function to get the top MAX_TOP_PROCESS_TO_REPORT CPU-consuming processes
def get_top_processes():
    processes = []
    for proc in psutil.process_iter(['pid', 'name', 'cpu_percent']):
        try:
            cpu_percent = proc.info['cpu_percent']
            if cpu_percent is not None and cpu_percent > 0:  # Only include active processes
                processes.append(proc.info)
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    processes.sort(key=lambda x: x['cpu_percent'], reverse=True)
    return processes[:MAX_TOP_PROCESS_TO_REPORT]

# Function to get the per-core CPU usage
def get_per_core_cpu_usage():
    return psutil.cpu_percent(percpu=True)

# Function to send a simple message to Discord via webhook
def send_discord_simple_message(machine_name, message):
    time_now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    simple_message = {
        "content": f"**Notification from {machine_name}**\n"
                   f"- *Time*: {time_now}\n"
                   f"- *Message*: {message}"
    }
    response = requests.post(DISCORD_WEBHOOK_URL, json=simple_message)
    if response.status_code != 204:
        print(f"Failed to send simple message. Status code: {response.status_code}")
        print(f"Response Text: {response.text}")  # Include additional details from the response body

# Function to send a message to Discord via webhook
def send_discord_message(avg_cpu_usage, cpu_samples, top_processes, per_core_usage, machine_name, pid):
    time_now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    message = {
        "content": f"**CPU Usage Spike Detected!**\n"
                   f"- *Time*: {time_now}\n"
                   f"- *Machine Name*: {machine_name}\n"
                   f"- *cpu-monitor script process ID*: {pid}\n"
                   f"- **Average CPU Usage**: {avg_cpu_usage}%\n"
                   f"- *Sample History*: {', '.join([f'{sample}%' for sample in cpu_samples])}\n"
                   f"- **Per-Core CPU Usage**: {', '.join([f'{core}%' for core in per_core_usage])}\n"
                   f"- **Top Processes**:\n"
                   f"{''.join([f'  - **PID: {proc["pid"]}** - {proc["name"]} ({proc["cpu_percent"]}%)\n' for proc in top_processes])}"
    }
    response = requests.post(DISCORD_WEBHOOK_URL, json=message)
    if response.status_code != 204:
        print(f"Failed to send message. Status code: {response.status_code}")
        print(f"Response Text: {response.text}")  # Include additional details from the response body

# Main function to monitor CPU usage and send alerts
def monitor_cpu_usage(cpu_threshold, check_interval, sample_history):
    # Print the arguments used
    print("Starting CPU Monitor with the following parameters:")
    print(f"- CPU Threshold: {cpu_threshold}%")
    print(f"- Check Interval: {check_interval} seconds")
    print(f"- Sample History: {sample_history} samples")
    print(f"- Report Limit: {REPORT_LIMIT} times")
    print(f"- Cool down after report limit: {COOLDOWN_TIME} seconds")
    print(f"- MAX_CYCLES: {MAX_CYCLES} times\n")

    # Get the machine's hostname
    machine_name = socket.gethostname()

    # Get the process ID of the running script
    pid = os.getpid()

    # Initialize a list to store the previous CPU samples
    cpu_samples = []
    
    # Reporting cycle variables
    report_count = 0
    cycle_count = 0
    threshold_crossed = False  # Flag to track if CPU threshold has been crossed

    while True:  # Infinite loop to continue monitoring after cycles
        # Get overall CPU usage and per-core CPU usage
        overall_cpu_usage = psutil.cpu_percent(interval=1)
        per_core_usage = get_per_core_cpu_usage()

        # Add the current CPU sample to the list
        cpu_samples.append(overall_cpu_usage)

        # Keep only the last SAMPLE_HISTORY samples
        if len(cpu_samples) > sample_history:
            cpu_samples.pop(0)

        # Calculate the average of the last SAMPLE_HISTORY samples
        average_cpu_usage = sum(cpu_samples) / len(cpu_samples)

        # Get the top MAX_TOP_PROCESS_TO_REPORT processes consuming CPU
        top_processes = get_top_processes()

        # If the average CPU usage exceeds the threshold, report to Discord
        if average_cpu_usage > cpu_threshold:
            print(f"Spike detected! Average CPU usage: {average_cpu_usage}% at {datetime.now()}")
            send_discord_message(
                avg_cpu_usage=average_cpu_usage,
                cpu_samples=cpu_samples,
                top_processes=top_processes,
                per_core_usage=per_core_usage,
                machine_name=machine_name,
                pid=pid
            )
            report_count += 1
            threshold_crossed = True  # Set the flag to True since threshold was crossed

            # If report limit is reached, apply cooldown
            if report_count >= REPORT_LIMIT:
                report_count = 0  # Reset report counter after cooldown
                # Increment cycle count after the sleep
                cycle_count += 1
                print(f"Report limit reached. Cycle {cycle_count}. Cooling down for {COOLDOWN_TIME} seconds...")
                time.sleep(COOLDOWN_TIME)

        # Report CPU usage restoration only if threshold was crossed previously
        elif average_cpu_usage < cpu_threshold and threshold_crossed:
            cycle_count = 0  # Reset cycle count if the CPU usage goes below the threshold
            print(f"CPU usage restored.")
            send_discord_simple_message(machine_name=machine_name, message="CPU usage restored.")
            threshold_crossed = False  # Reset the flag after reporting

        # Sleep for CHECK_INTERVAL seconds before the next check
        time.sleep(check_interval)

        # If maximum cycles are reached, sleep for 15 minutes and continue
        if cycle_count >= MAX_CYCLES:
            print(f"Completed {MAX_CYCLES} cycles. Sleeping for 15 minutes before continuing.")
            time.sleep(900)  # Sleep for 15 minutes (900 seconds)

            cycle_count = 0  # Reset cycle count after sleep

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Monitor CPU usage and report spikes.")
    parser.add_argument("--cpu-threshold", type=float, default=DEFAULT_CPU_THRESHOLD,
                        help="CPU usage threshold to trigger a spike alert (default: 10.0)")
    parser.add_argument("--check-interval", type=int, default=DEFAULT_CHECK_INTERVAL,
                        help="Interval (in seconds) between checks (default: 10 seconds)")
    parser.add_argument("--sample-history", type=int, default=DEFAULT_SAMPLE_HISTORY,
                        help="Number of previous samples to consider for average CPU usage (default: 4)")

    args = parser.parse_args()
    monitor_cpu_usage(cpu_threshold=args.cpu_threshold, check_interval=args.check_interval, sample_history=args.sample_history)
