from enum import Enum
import argparse
import socket

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _sock = None
    # ******************** METHODS *******************

    @staticmethod
    def connect_to_server():
        try:
            client._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._sock.connect((client._server, client._port))
            print("Connected to server: " + client._server + " on port: " + str(client._port))
        except Exception as e:
            print("Exception: " + str(e))
            return client.RC.ERROR

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
    def  register(user) :
        #  Write your code here
        if client.connect_to_server() == client.RC.ERROR:
            print("Error connecting to server")
            return client.RC.ERROR

        try:
            # Send REGISTER command
            client._sock.sendall(b'REGISTER\0')

            # Send username
            client._sock.sendall(user.encode() + b'\0')

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
            print("Exception: " + str(e))
            return client.RC.ERROR


    @staticmethod
    def  unregister(user) :
        #  Write your code here
        if client.connect_to_server() == client.RC.ERROR:
            print("Error connecting to server")
            return client.RC.ERROR

        try:
            # Send REGISTER command
            client._sock.sendall(b'UNREGISTER\0')

            # Send username
            client._sock.sendall(user.encode() + b'\0')

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
            print("Exception: " + str(e))
            return client.RC.ERROR


    @staticmethod
    def connect(user):
        try:
            # Establish a connection with the server
            client.connect_to_server()

            # Send 'CONNECT' command
            client._sock.sendall(b'CONNECT\0')

            # Send user name
            client._sock.sendall(user.encode() + b'\0')

            # Obtain a free port
            listening_port = client.get_free_port()

            # Send listening port
            client._sock.sendall(listening_port.encode() + b'\0')

            # Receive response
            response = int(client.readString())

            if response == client.RC.OK.value:
                print("CONNECT OK")
            elif response == client.RC.ERROR.value:
                print("CONNECT FAIL, USER DOES NOT EXIST")
            elif response == client.RC.USER_ERROR.value:
                print("USER ALREADY CONNECTED")
            else:
                print("CONNECT FAIL")

            client._sock.close()
            return client.RC(response)
        except Exception as e:
            print("Exception: " + str(e))
            return client.RC.ERROR




    @staticmethod
    def  disconnect(user) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  publish(fileName,  description) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  delete(fileName) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  listusers() :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  listcontent(user) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  getfile(user,  remote_FileName,  local_FileName) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.listusers()
                        else :
                            print("Syntax error. Use: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.listcontent(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
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
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here

        client.shell()
        print("+++ FINISHED +++")


if __name__=="__main__":
    client.main([])