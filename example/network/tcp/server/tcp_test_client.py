"""
tcp_test_client.py

TCP Client Test Script for main.cpp
Supports:
  1) Interactive Mode (IPv4 or IPv6)
  2) Multi-client Automated Simulation Mode (IPv4 + IPv6 concurrent connections)
"""

import sys
import time
import socket
import argparse
import threading


def receive_loop(sock: socket.socket, stop_event: threading.Event, client_label: str = ""):
    prefix = f"[{client_label}] " if client_label else ""
    while not stop_event.is_set():
        try:
            data = sock.recv(4096)
            if not data:
                print(f"\n{prefix}[Notice] Server closed the connection.")
                break
            msg = data.decode("utf-8", errors="replace")
            print(f"\n{prefix}[Received]: {msg}\n> ", end="", flush=True)
        except (ConnectionResetError, OSError):
            break


def run_interactive(ip_version: str):
    if ip_version == "4":
        family = socket.AF_INET
        host = "127.0.0.1"
        port = 12345
        label = "IPv4"
    elif ip_version == "6":
        family = socket.AF_INET6
        host = "::1"
        port = 12346
        label = "IPv6"
    else:
        print("[Error] Invalid IP version. Please choose '4' or '6'.")
        return

    sock = socket.socket(family, socket.SOCK_STREAM)
    stop_event = threading.Event()

    try:
        sock.connect((host, port))
        print(f"[{label}] Connected to {host}:{port}")
        print("Type message to send. Enter 'exit' to disconnect.\n")

        recv_thread = threading.Thread(
            target=receive_loop,
            args=(sock, stop_event, label),
            daemon=True
        )
        recv_thread.start()

        while True:
            msg = input("> ")
            if msg.strip().lower() == "exit":
                break
            if msg:
                sock.sendall(msg.encode("utf-8"))

    except ConnectionRefusedError:
        print(f"[Error] Failed to connect to {host}:{port}. Ensure main.cpp server is running.")
    except KeyboardInterrupt:
        print("\nExiting client...")
    finally:
        stop_event.set()
        sock.close()
        print(f"[{label}] Connection closed.")


def simulate_client(family, host: str, port: int, client_id: str, duration: int):
    try:
        with socket.socket(family, socket.SOCK_STREAM) as sock:
            sock.connect((host, port))
            print(f"[{client_id}] Connected to {host}:{port}")

            # Send identification message
            init_msg = f"Hello from {client_id}"
            sock.sendall(init_msg.encode("utf-8"))

            sock.settimeout(1.0)
            start_time = time.time()
            while time.time() - start_time < duration:
                try:
                    data = sock.recv(4096)
                    if data:
                        print(f"[{client_id}] Broadcast received: {data.decode('utf-8', errors='replace')}")
                except socket.timeout:
                    continue
                except OSError:
                    break

            print(f"[{client_id}] Disconnected.")
    except Exception as e:
        print(f"[{client_id}] Error: {e}")


def run_multi_simulation(ipv4_count: int = 2, ipv6_count: int = 2, duration: int = 15):
    print(f"Starting simulation with {ipv4_count} IPv4 clients and {ipv6_count} IPv6 clients for {duration}s...")
    threads = []

    # Spawn IPv4 clients
    for i in range(1, ipv4_count + 1):
        t = threading.Thread(
            target=simulate_client,
            args=(socket.AF_INET, "127.0.0.1", 12345, f"IPv4-Client-{i}", duration)
        )
        threads.append(t)

    # Spawn IPv6 clients
    for i in range(1, ipv6_count + 1):
        t = threading.Thread(
            target=simulate_client,
            args=(socket.AF_INET6, "::1", 12346, f"IPv6-Client-{i}", duration)
        )
        threads.append(t)

    for t in threads:
        t.start()

    for t in threads:
        t.join()

    print("Multi-client simulation completed.")


def main():
    parser = argparse.ArgumentParser(description="TCP Test Client for main.cpp")
    parser.add_argument(
        "--mode",
        choices=["interactive", "simulate"],
        default=None,
        help="Execution mode: 'interactive' or 'simulate'"
    )
    parser.add_argument(
        "--ip",
        choices=["4", "6"],
        default="4",
        help="IP version for interactive mode (default: 4)"
    )
    parser.add_argument(
        "--duration",
        type=int,
        default=15,
        help="Duration in seconds for simulation mode (default: 15)"
    )
    parser.add_argument(
        "--clients",
        type=int,
        default=2,
        help="Number of clients per IP version in simulation mode (default: 2)"
    )

    args = parser.parse_args()

    if args.mode is None:
        print("=== TCP Test Client Menu ===")
        print("1. Interactive IPv4 Client (127.0.0.1:12345)")
        print("2. Interactive IPv6 Client (::1:12346)")
        print("3. Multi-client Simulation (IPv4 + IPv6 concurrent)")
        choice = input("Select option (1-3): ").strip()

        if choice == "1":
            run_interactive("4")
        elif choice == "2":
            run_interactive("6")
        elif choice == "3":
            run_multi_simulation(duration=15)
        else:
            print("[Error] Invalid choice.")
    elif args.mode == "interactive":
        run_interactive(args.ip)
    elif args.mode == "simulate":
        run_multi_simulation(ipv4_count=args.clients, ipv6_count=args.clients, duration=args.duration)


if __name__ == "__main__":
    main()


