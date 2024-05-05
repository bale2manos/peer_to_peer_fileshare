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

# Start the client shell
process1 = subprocess.Popen(['python3', 'client.py', '-s', 'localhost', '-p', '3333'], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

# Start a separate thread to print output
output_thread_1 = threading.Thread(target=print_output, args=(process1,))
output_thread_1.start()

error_thread = threading.Thread(target=print_errors, args=(process1,))
error_thread.start()

time.sleep(2)

# Define instructions with delays
instructions1 = [
    "REGISTER usuario1"
]

# Send instructions with a delay of 3 seconds between each
for instruction1 in instructions1:
    send_instruction(instruction1, process1)
    time.sleep(1)

# Close the client shell
process1.stdin.close()
