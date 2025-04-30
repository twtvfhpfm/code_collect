import socket
import struct
import json
import netifaces
import struct

def get_mac_address(interface_name):
    try:
        addrs = netifaces.ifaddresses(interface_name)
        mac_address = addrs[netifaces.AF_LINK][0]['addr']
        mac_address = mac_address.replace(':','')
        return mac_address
    except KeyError:
        return None

MCAST_GRP = '239.3.3.3'
MCAST_PORT = 12345
RECV_BUFFER_SIZE = 1024
REPLY_MESSAGE = "Acknowledgment from server"

def join_multicast_group(sock):
    """加入组播组"""
    mreq = struct.pack("4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton('192.168.1.169'))
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

def create_multicast_socket():
    """创建组播套接字"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 允许地址重用
    sock.bind(('', MCAST_PORT)) # 绑定到组播端口
    join_multicast_group(sock) # 加入组播组
    return sock

def receive_message_from_server(host, port):
    # 创建一个 TCP 套接字
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        try:
            # 连接到服务器
            sock.connect((host, port))
            print(f"Connected to server at {host}:{port}")

            # 接收数据
            while True:
                data = sock.recv(4)  # 每次接收 1024 字节
                if not data:
                    break  # 如果没有收到数据，说明连接已关闭
                data_size = struct.unpack('<I', data)[0]  # '<I' 表示小端的无符号整数
                print("Received:", data_size)
                while data_size > 0:
                    data = sock.recv(data_size)
                    if not data:
                        break
                    data_size -= len(data)

        except Exception as e:
            print(f"An error occurred: {e}")
            return

def receive_and_reply(sock):
    """接收消息并回复"""
    print(f"Listening for multicast messages on {MCAST_GRP}:{MCAST_PORT}...")
    while True:
        try:
            data, addr = sock.recvfrom(RECV_BUFFER_SIZE)  # 从组播组接收数据
            print(f"Received message from {addr}: {data.decode('utf-8')}")
            
            jdata = json.loads(data)
            if jdata['cmd'] == 0:#bind
                reply_data = {'cmd':1, 'id': get_mac_address("wlxbc307eab1114")}
                sock.sendto(json.dumps(reply_data).encode('utf-8'), addr)
                print(f"Sent reply to {addr}: {reply_data}")
            elif jdata['cmd'] == 2:#cast
                reply_data = {'cmd':3, 'id': get_mac_address("wlxbc307eab1114")}
                sock.sendto(json.dumps(reply_data).encode('utf-8'), addr)
                print(f"Sent reply to {addr}: {reply_data}")
                receive_message_from_server(addr[0], jdata['port'])

        except KeyboardInterrupt:
            print("\nStopped listening.")
            break
        # except Exception as e:
        #     print(f"An error occurred: {e}")

def main():
    sock = create_multicast_socket()
    receive_and_reply(sock)

if __name__ == "__main__":
    main()
