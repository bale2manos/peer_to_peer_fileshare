import subprocess
import time
import threading
import os


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

# Remove all the files and subfolders of ../database
subprocess.run(['rm', '-rf', './local_storage'])
subprocess.run(['mkdir', './local_storage'])

# Use try-except to suppress error message
try:
    subprocess.run(['rm', './current_username.txt'], stderr=subprocess.DEVNULL)
except FileNotFoundError:
    pass

# Start the client shells
processes = []
output_threads = []
error_threads = []
processes_calls = [['python3', 'client.py', '-s', 'localhost', '-p', '4500'],
                   ['python3', 'client.py', '-s', 'localhost', '-p', '4500']]

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

for i in [1,2]:
    user = "usuario" + str(i)
    # Create the directory path
    directory_path = os.path.join(os.getcwd(), 'local_storage', user)

    # Create the directory if it doesn't exist
    if not os.path.exists(directory_path):
        os.makedirs(directory_path)

    # Crea fichero con el contenido en local_storage
    path = os.getcwd() + '/local_storage/' + user + '/' + 'title'+ str(i)
    with open(path, 'w') as file:
        file.write("contenido del fichero " + str(i) + " de usuario " + str(i) + "\n")


# Define instructions with delays
instructions = [
    ["REGISTER usuario1", "CONNECT usuario1", "PUBLISH title1 description1", "DISCONNECT usuario1", "QUIT"],
    ["REGISTER usuario2", "CONNECT usuario2", "LIST_CONTENT usuario1", "GET_FILE usuario1 title1 title1copia", "QUIT"]

]

# Send instructions with a delay of 3 seconds between each
for i in range(len(instructions[0])):
    for j in range(len(instructions)):
        send_instruction(instructions[j][i], processes[j])
        time.sleep(0.2)

# Close the client shells
for process in processes:
    process.stdin.close()
