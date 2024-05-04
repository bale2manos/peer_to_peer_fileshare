import subprocess
import time
import threading

# Function to send instructions to the client shell and print them
def send_instruction(instruction, process):
    print(instruction)
    process.stdin.write(instruction + '\n')
    process.stdin.flush()

# Function to print output from client shell
def print_output(process):
    for line in process.stdout:
        print(line.strip())

# Function to handle errors and print stderr output
def print_errors(process):
    for line in process.stderr:
        print(line.strip())
    process.stderr.close()

# Remove all the files and subfolders of ../database
subprocess.run(['rm', '-rf', './database'])
subprocess.run(['mkdir', './database'])

try:
    subprocess.run(['rm', './current_username.txt'], stderr=subprocess.DEVNULL)
except FileNotFoundError:
    pass

# Start the client shells
processes = []
output_threads = []
error_threads = []

for args in [['python3', 'client.py'], ['python3', 'client.py', '-s', 'localhost'], ['python3', 'client.py', '-p', '4500']]:
    process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    processes.append(process)

    # Start a separate thread to print output and errors
    output_thread = threading.Thread(target=print_output, args=(process,))
    output_thread.start()
    output_threads.append(output_thread)

    error_thread = threading.Thread(target=print_errors, args=(process,))
    error_thread.start()
    error_threads.append(error_thread)

# Wait for the client shells to start
time.sleep(1)  # Adjust this delay according to your project's startup time

# Close the client shells
for process in processes:
    process.stdin.close()
