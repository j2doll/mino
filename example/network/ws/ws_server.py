import asyncio
import websockets # pip install websockets

async def echo_handler(websocket):
    # 클라이언트 접속 로그
    print(f"[CONNECTED] Client connected: {websocket.remote_address}")
    try:
        async for message in websocket:
            print(f"[RECEIVED] {message}")
            response = f"Echo: {message}"
            await websocket.send(response)
            print(f"[SENT] {response}")
    except websockets.exceptions.ConnectionClosedOK:
        # 정상 연결 종료 처리
        print("[CLOSED] Connection closed normally.")
    except websockets.exceptions.ConnectionClosedError as e:
        # 비정상 연결 종료 에러 처리
        print(f"[ERROR] Connection closed with error: {e}")
    finally:
        # 최종 연결 해제 로그
        print(f"[DISCONNECTED] Client disconnected: {websocket.remote_address}")

async def main():
    host = "localhost"
    port = 8765
    
    # 서버 구동
    async with websockets.serve(echo_handler, host, port):
        print(f"WS server started: ws://{host}:{port}")
        # 서버 유지를 위한 대기
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())
