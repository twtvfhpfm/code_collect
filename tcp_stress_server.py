import threading
import socket

def worker(skt):
    try:
        while True:
            resp = skt.recv(2000)
            if resp:
                skt.sendall(resp)
            else:
                break
    except Exception as e:
        print("worker exception: ", e)

    skt.close()


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(('', 56789))
    server.listen(1000)
    threads = []
    while True:
        client, address = server.accept()
        found = False
        for i in range(len(threads)):
            if not threads[i].is_alive():
                threads[i].join()
                threads[i] = threading.Thread(target=worker, args=(client,))
                print("start new worker")
                threads[i].start()
                found = True
                break

        if not found:
            thread = threading.Thread(target=worker, args=(client,))
            print("start new worker")
            thread.start()
            threads.append(thread)
        print("threads num: ", len(threads))


main()





