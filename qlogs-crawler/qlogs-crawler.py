import os
import time
import shutil
import argparse
import json
import datetime
import signal
import sys
from opensearchpy import OpenSearch
from opensearchpy import exceptions


def parse_log_record(record):
    parts = record.split("|")
    if len(parts) >= 5:
        # timestamp = parts[0].strip()
        unix_timestamp = int(parts[0].strip())  # Convert to int
        timestamp = datetime.datetime.utcfromtimestamp(unix_timestamp).isoformat() + 'Z'  # Convert to UTC ISO 8601 format

        level = parts[1].strip()
        source = parts[2].strip()
        pid = parts[3].strip() or "nopid"  # Default to "nopid" if pid is empty
        roomid = parts[4].strip() or "noroom"  # Default to "1noroom" if roomid is empty
        message = "|".join(part.strip() for part in parts[5:])  # Combine remaining parts into the message
        return {
            "timestamp": timestamp,
            "level": level,
            "pid": pid,
            "roomid": roomid,
            "message": message,
            "source": source
        }

    return None

def send_batch(es, index_name, batch):
    if batch:
        try:
            # Prepare the bulk actions without _source
            actions = []
            for log_data in batch:
                action = {
                    "index": {"_index": index_name}
                }
                actions.append(action)  # Add the index action
                actions.append(log_data)  # Add the actual log data

            # Debug print the actions being sent to OpenSearch
            # print("Sending the following batch to OpenSearch:", json.dumps(actions, indent=2))

            response = es.bulk(body=actions)
            # print(f"Bulk insert response: {response}")

            # Check for errors in the response
            if response['errors']:
                print("Bulk insert encountered errors:")
                for item in response['items']:
                    if 'error' in item.get('index', {}):
                        print(f"Error: {item['index']['error']}")
                return False  # Indicate failure in sending batch

            # Check individual item results for errors
            for item in response['items']:
                if 'error' in item.get('index', {}):
                    print(f"Error in item: {item['index']['error']}")
                    return False  # Indicate failure for individual item

            return True  # Return True if successful
        except exceptions.NotFoundError as e:
            print(f"Index not found error: {e}")
        except exceptions.ConflictError as e:
            print(f"Conflict error: {e}")
        except Exception as e:
            print(f"An error occurred during bulk insert: {e}")
    
    return False  # Return False if there was an error

def move_file(file_path, target_folder):
    if not os.path.exists(target_folder):
        os.makedirs(target_folder)

    target_path = os.path.join(target_folder, os.path.basename(file_path))
    try:
        shutil.move(file_path, target_path)
        print(f"File moved to: {target_path}")
    except Exception as e:
        print(f"Error moving file {file_path} to {target_folder}: {e}")


def process_logs(es, index_name, log_folder_path, max_batch_size):
    print(f"Current working directory: {os.getcwd()}")  # Print the current working directory

    # Ensure the log folder exists
    if not os.path.exists(log_folder_path):
        print(f"Error: The folder path '{log_folder_path}' does not exist. returning !!!")
        return

    for child in os.listdir(log_folder_path):
        child_path = os.path.join(log_folder_path, child)

        if os.path.isdir(child_path):
            logged_folder = os.path.join(child_path, "logged")
            log_failed_folder = os.path.join(child_path, "log_failed")

            for log_file in os.listdir(child_path):
                log_file_path = os.path.join(child_path, log_file)

                if os.path.isfile(log_file_path) and log_file.endswith('.log'):
                    batch = []
                    success = True  # Flag to track successful processing
                    try:
                        with open(log_file_path, "r") as file:
                            for line in file:
                                log_data = parse_log_record(line.strip())
                                if log_data:
                                    batch.append(log_data)

                                if len(batch) >= max_batch_size:
                                    if not send_batch(es, index_name, batch):
                                        success = False  # Batch send failed
                                    batch = []  # Clear the batch after sending

                        # Send remaining batch if any
                        if batch:
                            if not send_batch(es, index_name, batch):
                                success = False  # Batch send failed

                    except Exception as e:
                        print(f"Error processing file {log_file_path}: {e}")
                        success = False  # Set success to False on exception

                    # Move the file based on the success flag
                    if success:
                        move_file(log_file_path, logged_folder)
                    else:
                        move_file(log_file_path, log_failed_folder)

def signal_handler(sig, frame):
    print("Gracefully shutting down...")
    sys.exit(0)

def main():
    # Set up signal handler for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)

    parser = argparse.ArgumentParser(
        description="Process log files and send them to OpenSearch."
    )
    parser.add_argument(
        "folder_path",
        help="Path to the folder containing log files"
    )
    parser.add_argument(
        "index_name",
        help="OpenSearch index name to send logs"
    )
    parser.add_argument(
        "host_port",
        help="OpenSearch host and port in the format host:port"
    )
    parser.add_argument(
        "--batch_size",
        type=int,
        default=50,
        help="Maximum number of records to send in one batch (default: 50)"
    )
    parser.add_argument(
        "--interval",
        type=int,
        default=60,
        help="Interval in seconds to scan for new logs (default: 60)"
    )

    args = parser.parse_args()

    # Split host and port
    host, port = args.host_port.split(":")
    port = int(port)

    # Connect to OpenSearch
    es = OpenSearch(
        hosts=[{'host': host, 'port': int(port)}],
        http_auth=('admin', 'Fr0gmoon@123'),  # Update with your credentials
        use_ssl=True,
        verify_certs=False,
        timeout=10,  # Set a timeout for the requests
        max_retries=3  # Set max retries for network calls
    )

    while True:
        process_logs(es, args.index_name, args.folder_path, args.batch_size)
        print(f"Waiting for {args.interval} seconds before the next scan...")
        time.sleep(args.interval)


if __name__ == "__main__":
    main()
