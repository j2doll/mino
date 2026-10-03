import asyncio

HOST_IPV4 = "127.0.0.1"
PORT_IPV4 = 12345

HOST_IPV6 = "::1"
PORT_IPV6 = 12346


async def handle_client(reader: asyncio.StreamReader, writer: asyncio.StreamWriter, proto_label: str):
    addr = writer.get_extra_info("peername")
    print(f"[{proto_label}] Connection accepted from: {addr}")

    # Send a welcome message right after connection
    writer.write(f"[{proto_label} Server] Connection Established!\n".encode("utf-8"))
    await writer.drain()

    try:
        while True:
            # Read incoming data (up to 1024 bytes)
            data = await reader.read(1024)
            if not data:
                print(f"[{proto_label}] Client disconnected: {addr}")
                break

            message = data.decode("utf-8", errors="replace")
            print(f"[{proto_label}] Received: {message.strip()}")

            # Echo response to verify client's on_receive handler
            response = f"[ACK from {proto_label}] Echo: {message}"
            writer.write(response.encode("utf-8"))
            await writer.drain()

    except asyncio.CancelledError:
        pass
    except ConnectionResetError:
        print(f"[{proto_label}] Connection forcibly closed by client: {addr}")
    except Exception as e:
        print(f"[{proto_label}] Error occurred: {e}")
    finally:
        writer.close()
        await writer.wait_closed()


async def main():
    # Start IPv4 server
    server_ipv4 = await asyncio.start_server(
        lambda r, w: handle_client(r, w, "IPv4"),
        HOST_IPV4,
        PORT_IPV4,
    )

    # Start IPv6 server
    server_ipv6 = await asyncio.start_server(
        lambda r, w: handle_client(r, w, "IPv6"),
        HOST_IPV6,
        PORT_IPV6,
    )

    print(f"[*] IPv4 Server listening on: {HOST_IPV4}:{PORT_IPV4}")
    print(f"[*] IPv6 Server listening on: [{HOST_IPV6}]:{PORT_IPV6}")
    print("[*] Press Ctrl+C to terminate.\n")

    async with server_ipv4, server_ipv6:
        await asyncio.gather(
            server_ipv4.serve_forever(),
            server_ipv6.serve_forever(),
        )


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[*] Server terminated.")

