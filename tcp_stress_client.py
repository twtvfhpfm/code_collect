import threading
import time
import random
import socket
import sys

server_addr = ""
server_port = 56789

def generate_data():
    length = random.randint(100, 1000)
    data_map = ['a','b','c','d','e','f','g', 'h']
    data = []
    for i in range(length):
        data.append(data_map[random.randint(0, 7)])

    return "".join(data)

def worker():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((server_addr, server_port))
    send_times = random.randint(4, 10);
    send_count = 0
    try:
        while send_count < send_times:
            s = generate_data()
            client.sendall(s.encode('utf-8'))
            response = client.recv(2000)
            if response.decode('utf-8') != s:
                print("response error")
            time.sleep(random.randint(1,60))
            send_count += 1
    except Exception as e:
        print("socket error: ", e)
    client.close()

def main():
    threads = []
    for i in range(1000):
        thread = threading.Thread(target=worker)
        # print("start new worker")
        threads.append(thread)
        thread.start()
    while True:
        for i in range(len(threads)):
            if not threads[i].is_alive():
                threads[i].join()
                threads[i] = threading.Thread(target=worker)
                # print("start new worker")
                threads[i].start()

        time.sleep(1)

if len(sys.argv) < 2:
    print("usage: server_ip")
    exit()

server_addr = sys.argv[1]
main()
