from enum import Enum
import argparse
import socket
import threading
import os
import zeep


class client:
    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum):
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** fATTRIBUTES ******************
    _server = None
    _port = -1
    _sock = None
    _listening_socket = None
    _listening_thread = None
    _conectado = True
    # _username is stored in the file in the same directory called current_username.txt
    _username = None
    if _username is None:
        try:
            with open("current_username.txt", "r") as file:
                _username = file.read()
        except:
            _username = None
    _list_users = {}

    # ******************** METHODS *******************

    @staticmethod
    def connect_to_server():
        try:
            client._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._sock.connect((client._server, client._port))
        except Exception as e:
            # print("Exception connect_to_server: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def get_current_timestamp():
        wsdl_url = "http://localhost:8000/?wsdl"
        soap = zeep.Client(wsdl=wsdl_url)
        result = soap.service.take_timestamp()
        return result

    @staticmethod
    def readString():
        a = ''
        while True:
            msg = client._sock.recv(1)
            if (msg == b'\0'):
                break
            a += msg.decode()
        return a

    @staticmethod
    def get_free_port():
        # Create a temporary socket
        temp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Bind the socket to an address with port 0
        temp_socket.bind(('localhost', 0))

        # Get the port number assigned by the operating system
        free_port = temp_socket.getsockname()[1]

        # Close the socket
        temp_socket.close()

        return free_port

    @staticmethod
    def listen_for_connections(listening_socket):
        try:
            listening_socket.listen(1)

            while client._conectado:
                # Accept incoming connections
                try:
                    connection, address = listening_socket.accept()
                except Exception as e:
                    # If errno 22 is raised, it means that the socket has been closed
                    if e.errno == 22:
                        break
                    print("Error accepting connection: ", e)
                    continue
                # Get port to send from the address

                # Receive first word, read from the connection until the first space
                operation = b''
                while True:
                    msg = connection.recv(1)
                    if (msg == b'\0'):
                        break
                    operation += msg

                # If the data is 'GET_FILE', send the file
                if operation.decode() == 'GET_FILE':
                    filename = ''
                    while True:
                        msg = connection.recv(1)
                        if (msg == b'\0'):
                            break
                        filename += msg.decode()
                    user_path = '/database/' + client._username
                    file_path = user_path + '/files/' + filename
                    current_path = os.getcwd()
                    path = current_path + file_path
                    if not os.path.isfile(path):
                        print("File not found")
                        data_to_send = b'1\0'
                        connection.sendall(data_to_send)
                        continue
                    data_to_send = b'0\0'
                    print("Sending file")
                    connection.sendall(data_to_send)

                    client.send_file(connection, path)

                    connection.close()

                # Close the connection
                connection.close()
            # exit thread
        except Exception as e:
            print("Exception listening_for_connections: " + str(e))

    @staticmethod
    def send_file(connection, file_name):
        try:
            # Open the file
            with open(file_name, 'rb') as file:
                # Read the file
                data = file.read(1024)

                # Send the file
                while data:
                    connection.send(data)
                    data = file.read(1024)
                connection.send(b'\0')
        except Exception as e:
            print("Exception send_file " + str(e))

    @staticmethod
    def register(user):
        #  Write your code here
        if client.connect_to_server() == client.RC.ERROR:
            print("REGISTER FAIL")
            return client.RC.ERROR

        try:
            # Send REGISTER command
            client._sock.sendall(b'REGISTER\0')

            c_timestamp = client.get_current_timestamp()

            client._sock.sendall(c_timestamp.encode() + b'\0')

            if user:
                # Send username
                client._sock.sendall(user.encode() + b'\0')
            else:
                client._sock.sendall(b'__NO_USER__\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("REGISTER OK")
            elif response == client.RC.ERROR.value:
                print("USERNAME IN USE")
            else:
                print("REGISTER FAIL")
            client._sock.close()
            return client.RC(response)
        except Exception as e:
            print("Exception register: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def unregister(user):
        #  Write your code here
        if client.connect_to_server() == client.RC.ERROR:
            print("UNREGISTER FAIl")
            return client.RC.ERROR

        try:
            # Send REGISTER command
            client._sock.sendall(b'UNREGISTER\0')

            c_timestamp = client.get_current_timestamp()

            client._sock.sendall(c_timestamp.encode() + b'\0')

            if user:
                # Send username
                client._sock.sendall(user.encode() + b'\0')
            else:
                client._sock.sendall(b'__NO_USER__\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("UNREGISTER OK")
            elif response == client.RC.ERROR.value:
                print("USER DOES NOT EXIST")
            else:
                print("UNREGISTER FAIL")
            client._sock.close()
            return client.RC(response)
        except Exception as e:
            print("Exception unregister: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def connect(user):
        try:

            # TODO logica y permisos de que usuario se conecta

            # Obtain a free port
            listening_port = client.get_free_port()  # TODO revisar si es necesario

            # Create a new socket
            client._listening_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Create a new thread to listen for incoming connections in that socket
            client._conectado = True
            client._listening_thread = threading.Thread(target=client.listen_for_connections,
                                                        args=(client._listening_socket,), )
            client._listening_thread.daemon = True

            if client._listening_socket:
                client._listening_thread.start()  # TODO inicia el hilo aunque ya existiera o aunque no este registrado

            # Establish a connection with the server
            res = client.connect_to_server()
            if res == client.RC.ERROR:
                print("CONNECT FAIL")
                return client.RC.ERROR

            # Send 'CONNECT' command
            client._sock.sendall(b'CONNECT\0')

            c_timestamp = client.get_current_timestamp()

            client._sock.sendall(c_timestamp.encode() + b'\0')

            if user:
                # Send username
                client._sock.sendall(user.encode() + b'\0')
            else:
                client._sock.sendall(b'__NO_USER__\0')

            # Send listening port
            client._sock.sendall(str(client._listening_socket.getsockname()[1]).encode() + b'\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("CONNECT OK")
                # We store the name of the user in the client
                client._username = user
                with open("current_username.txt", "w") as file:
                    file.write(user)
            elif response == client.RC.ERROR.value:
                print("CONNECT FAIL, USER DOES NOT EXIST")
                if client._listening_socket:
                    client._listening_socket.shutdown(socket.SHUT_RDWR)  # TODO comentar en la memoria
                    client._listening_socket.close()
                    client._listening_socket = None
            elif response == client.RC.USER_ERROR.value:
                print("USER ALREADY CONNECTED")
                if client._listening_socket:
                    client._listening_socket.shutdown(socket.SHUT_RDWR)
                    client._listening_socket.close()
                    client._listening_socket = None
            else:
                if client._listening_socket:
                    client._listening_socket.shutdown(socket.SHUT_RDWR)
                    client._listening_socket.close()
                    client._listening_socket = None
                print("CONNECT FAIL")

            client._sock.close()
            return client.RC(response)
        except Exception as e:
            print(" Exception connect: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def disconnect(user):
        #  Write your code here
        if client.connect_to_server() == client.RC.ERROR:
            print("DISCONNECT FAIL")
            return client.RC.ERROR

        try:
            # Parar la ejecución del hilo y destruirlo
            client._conectado = False

            # Cerrar el socket de escucha y esperar a que el hilo termine con manejo de errores
            if client._listening_socket:
                client._listening_socket.shutdown(socket.SHUT_RDWR)  # TODO comentar en la memoria
                client._listening_socket.close()
                client._listening_socket = None

            # Send DISCONNECT command
            if user and client._sock:
                client._sock.sendall(b'DISCONNECT\0')
                c_timestamp = client.get_current_timestamp()

                client._sock.sendall(c_timestamp.encode() + b'\0')

                # Send user name
                client._sock.sendall(user.encode() + b'\0')

                # Receive response
                response = int(client.readString())
                if response == client.RC.OK.value:
                    print("DISCONNECT OK")
                elif response == client.RC.ERROR.value:
                    print("DISCONNECT FAIL / USER DOES NOT EXIST")
                elif response == client.RC.USER_ERROR.value:
                    print("DISCONNECT FAIL / USER NOT CONNECTED")
                else:
                    print("DISCONNECT FAIL")
                client._sock.close()
                client._sock = None
                return client.RC(response)
        except Exception as e:
            print("Exception Disconnect: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def publish(fileName, description):
        if client.connect_to_server() == client.RC.ERROR:
            print("PUBLISH FAIL")
            return client.RC.ERROR

        try:
            # Send PUBLISH command
            client._sock.sendall(b'PUBLISH\0')

            c_timestamp = client.get_current_timestamp()

            client._sock.sendall(c_timestamp.encode() + b'\0')

            # Send userName from attribute
            user = client._username
            if user:
                # Send username
                client._sock.sendall(user.encode() + b'\0')
            else:
                client._sock.sendall(b'__NO_USER__\0')

            # Send fileName
            client._sock.sendall(fileName.encode() + b'\0')

            # Send description
            client._sock.sendall(description.encode() + b'\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("PUBLISH OK")
            elif response == client.RC.ERROR.value:
                print("PUBLISH FAIL USER, DOES NOT EXIST")
            elif response == client.RC.USER_ERROR.value:
                print("PUBLISH FAIL, USER NOT CONNECTED")
            elif response == 3:
                print("PUBLISH FAIL, CONTENT ALREADY PUBLISHED")
            else:
                print("PUBLISH FAIL")
            client._sock.close()
            return response
        except Exception as e:
            print("Exception publish: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def delete(fileName):
        if client.connect_to_server() == client.RC.ERROR:
            print("DELETE FAIL")
            return client.RC.ERROR

        try:
            # Send PUBLISH command
            client._sock.sendall(b'DELETE\0')

            c_timestamp = client.get_current_timestamp()

            client._sock.sendall(c_timestamp.encode() + b'\0')

            user = client._username
            if user:
                # Send username
                client._sock.sendall(user.encode() + b'\0')
            else:
                # Send a special marker indicating no user
                client._sock.sendall(b'__NO_USER__\0')

            # Send fileName
            client._sock.sendall(fileName.encode() + b'\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("DELETE OK")
            elif response == client.RC.ERROR.value:
                print("DELETE FAIL, USER DOES NOT EXITS")
            elif response == client.RC.USER_ERROR.value:
                print("DELETE FAIL, USER NOT CONNECTED")
            elif response == 3:
                print("DELETE FAIL, CONTENT NOT PUBLISHED")
            else:
                print("DELETE FAIL")
            client._sock.close()
            return response
        except Exception as e:
            print("Exception delete: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def listusers():
        #  Write your code here
        if client.connect_to_server() == client.RC.ERROR:
            print("LIST_USERS FAIL")
            return client.RC.ERROR

        try:
            # Send LIST_USERS command
            client._sock.sendall(b'LIST_USERS\0')
            c_timestamp = client.get_current_timestamp()

            client._sock.sendall(c_timestamp.encode() + b'\0')

            # Send username
            user = client._username
            if user:
                # Send username
                client._sock.sendall(user.encode() + b'\0')
            else:
                client._sock.sendall(b'__NO_USER__\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("LIST_USERS OK")
                n_connections = int(client.readString())
                if n_connections == 0:
                    print(client.readString())
                print("n_connections: " + str(n_connections))
                for connection in range(n_connections):
                    user_info = client.readString()
                    print("\t%s" % user_info)
                    user_info = user_info.split(' ')
                    user_name = user_info[0]
                    user_ip = user_info[1]
                    # from the port remove the las '\n' character
                    user_port = user_info[2][:-1]
                    client._list_users[user_name] = (user_ip, user_port)
            elif response == client.RC.ERROR.value:
                print("LIST_USERS FAIL, USER DOES NOT EXIST")
            elif response == client.RC.USER_ERROR.value:
                print("LIST_USERS FAIL, USER NOT CONNECTED")
            else:
                print("LIST_USERS FAIL")

            client._sock.close()
            return client.RC(response)
        except Exception as e:
            print("Exception list_users: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    @staticmethod
    def listcontent(owner):
        #  Write your code here
        if client.connect_to_server() == client.RC.ERROR:
            print("LIST_CONTENT FAIL")
            return client.RC.ERROR

        try:
            # Send LIST_CONTENT command
            client._sock.sendall(b'LIST_CONTENT\0')

            c_timestamp = client.get_current_timestamp()

            client._sock.sendall(c_timestamp.encode() + b'\0')

            # Send user operating
            user = client._username
            if user:
                # Send username
                client._sock.sendall(user.encode() + b'\0')
            else:
                client._sock.sendall(b'__NO_USER__\0')

            # Send owner of the content
            client._sock.sendall(owner.encode() + b'\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("LIST_CONTENT OK")
                n_files = int(client.readString())
                if n_files == 0:
                    print(client.readString())
                for file in range(n_files):
                    print(client.readString())
            elif response == client.RC.ERROR.value:
                print("LIST_CONTENT FAIL, USER DOES NOT EXIST")
            elif response == client.RC.USER_ERROR.value:
                print("LIST_CONTENT FAIL, USER NOT CONNECTED")
            elif response == 3:
                print("LIST_CONTENT FAIL, REMOTE USER DOES NOT EXIST")
            else:
                print("LIST_CONTENT FAIL")
            client._sock.close()
            return response
        except:
            print("Exception list_content: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

        return client.RC.ERROR

    @staticmethod
    def getfile(owner, remote_FileName, local_FileName):
        # Send user operating
        user = client._username
        try:
            # Check if the user is in the list of users
            if owner not in client._list_users.keys():
                if client.connect_to_server() == client.RC.ERROR:
                    print("GET_FILE FAIL")
                    return client.RC.ERROR
                # SEND GET_FILE command
                client._sock.sendall(b'GET_FILE\0')

                if user:
                    # Send username
                    client._sock.sendall(user.encode() + b'\0')
                else:
                    client._sock.sendall(b'\0')

                # Send user owner
                if owner:
                    # Send username
                    client._sock.sendall(owner.encode() + b'\0')
                else:
                    client._sock.sendall(b'\0')

                # Receive response
                server_response = int(client.readString())
                if server_response != client.RC.OK.value:
                    print("GET_FILE FAIL")
                    client._sock.close()
                    return 2
                # Get client's ip and port
                client_address = client.readString()
                client_port = int(client.readString())
                client._sock.close()

            else:  # User in list of users
                client_address = client._list_users[owner][0]
                client_port = int(client._list_users[owner][1])

            # Connect to client
            try:
                # Connect to the client
                client._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                client._sock.connect((client_address, client_port))
            except:
                # If Ip has changed try to get the new ip
                # SEND GET_FILE command
                client._sock.sendall(b'GET_FILE\0')

                # Send user operating
                user = client._username
                if user:
                    # Send username
                    client._sock.sendall(user.encode() + b'\0')
                else:
                    client._sock.sendall(b'\0')

                # Send user owner
                client._sock.sendall(owner.encode() + b'\0')

                # Receive response
                server_response = int(client.readString())
                if server_response != client.RC.OK.value:
                    print("GET_FILE FAIL")
                    client._sock.close()
                    return 2
                # Get client's ip and port
                client_address = client.readString()
                client_port = int(client.readString())
                client._sock.close()
                try:

                    client._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    client._sock.connect((client_address, client_port))
                except:
                    print("GET_FILE FAIL")
                    client._sock.close()
                    return 2

            # Send operation

            client._sock.sendall(b'GET_FILE\0')

            # Send remote file name
            client._sock.sendall(remote_FileName.encode() + b'\0')

            # Receive response
            client_response = int(client.readString())

            if client_response == client.RC.OK.value:
                print("GET_FILE OK")
                # Open the file
                local_filePath = os.getcwd() + '/database/' + client._username + '/files/' + local_FileName
                with open(local_filePath, 'wb') as file:
                    # Read the file
                    data = client.readString()
                    # Write the file
                    file.write(data.encode())
            elif client_response == client.RC.ERROR.value:
                print("GET_FILE FAIL / FILE NOT EXIST")
            else:
                print("GET_FILE FAIL")
            client._sock.close()
            return client.RC(client_response)
        except Exception as e:
            print("Exception get_file: " + str(e))
            if client._sock:
                client._sock.close()
            return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True):
            try:
                command = input("c> ")
                if not command:
                    continue
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0] == "REGISTER"):
                        if (len(line) == 2):
                            client.register(line[1])
                        else:
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif (line[0] == "UNREGISTER"):
                        if (len(line) == 2):
                            client.unregister(line[1])
                        else:
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif (line[0] == "CONNECT"):
                        if (len(line) == 2):
                            client.connect(line[1])
                        else:
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif (line[0] == "PUBLISH"):
                        if (len(line) >= 3):
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else:
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif (line[0] == "DELETE"):
                        if (len(line) == 2):
                            client.delete(line[1])
                        else:
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif (line[0] == "LIST_USERS"):
                        if (len(line) == 1):
                            client.listusers()
                        else:
                            print("Syntax error. Use: LIST_USERS")

                    elif (line[0] == "LIST_CONTENT"):
                        if (len(line) == 2):
                            client.listcontent(line[1])
                        else:
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif (line[0] == "DISCONNECT"):
                        if (len(line) == 2):
                            client.disconnect(line[1])
                        else:
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif (line[0] == "GET_FILE"):
                        if (len(line) == 4):
                            client.getfile(line[1], line[2], line[3])
                        else:
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif (line[0] == "QUIT"):
                        if (len(line) == 1):
                            client.disconnect(client._username)
                            break
                        else:
                            print("Syntax error. Use: QUIT")
                    else:
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception shell: " + str(e))
                break

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage():
        print("Usage: python3 client.py -s <server> -p <port>")

    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def parseArguments(argv):
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;

        client._server = args.s
        client._port = args.p

        return True

    # ******************** MAIN *********************
    @staticmethod
    def main(argv):
        if (not client.parseArguments(argv)):
            client.usage()
            return

        client.shell()
        print("+++ FINISHED +++")


if __name__ == "__main__":
    client.main([])
