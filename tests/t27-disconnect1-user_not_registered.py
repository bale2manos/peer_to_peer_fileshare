import subprocess
import time
import threading


# Function to send instructions to the client shell and print them
def send_instruction(instruction, process):
    process.stdin.write(instruction + '\n')
    process.stdin.flush()
    print(instruction)


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

# Use try-except to suppress error message
try:
    subprocess.run(['rm', './current_username.txt'], stderr=subprocess.DEVNULL)
except FileNotFoundError:
    pass

# Start the client shells
processes = []
output_threads = []
error_threads = []
processes_calls = [['python3', 'client.py', '-s', 'localhost', '-p', '4500']]

for args in processes_calls:
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

# Define instructions with delays
instructions = [
    ["DISCONNECT usuario2", "QUIT"]
]

# Send instructions with a delay of 3 seconds between each
for i in range(len(instructions[0])):
    for j in range(len(instructions)):
        send_instruction(instructions[j][i], processes[j])
        time.sleep(0.2)

# Close the client shells
for process in processes:
    process.stdin.close()
