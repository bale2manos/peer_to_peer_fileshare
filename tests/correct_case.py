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

# Remove all the files and subfolders of ../database
subprocess.run(['rm', '-rf', './database'])
subprocess.run(['mkdir', './database'])

# Start the client shell
process1 = subprocess.Popen(['python3', 'client.py', '-s', 'localhost', '-p', '4500'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
process2 = subprocess.Popen(['python3', 'client.py', '-s', 'localhost', '-p', '4500'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

# Start a separate thread to print output
output_thread_1 = threading.Thread(target=print_output, args=(process1,))
output_thread_1.start()

output_thread_2 = threading.Thread(target=print_output, args=(process2,))
output_thread_2.start()

# Wait for the client shell to start
time.sleep(1)  # Adjust this delay according to your project's startup time

# Define instructions with delays
instructions1 = [
    "REGISTER usuario1",
    "CONNECT usuario1",
    "PUBLISH title1 contenido muy interesante",
    "LIST_CONTENT usuario2",
    "GET_FILE usuario2 title2 title2_copiado",
    "QUIT"
]

instructions2 = [
    "REGISTER usuario2",
    "CONNECT usuario2",
    "PUBLISH title2 contenido muy interesante",
    "LIST_USERS",
    "GET_FILE usuario1 title1 title1_copiado",
    "QUIT"
]

# Send instructions with a delay of 3 seconds between each
for instruction1, instruction2 in zip(instructions1, instructions2):
    send_instruction(instruction1, process1)
    time.sleep(0.5)
    send_instruction(instruction2, process2)
    time.sleep(0.5)


# Close the client shell
process1.stdin.close()
process2.stdin.close()
