import asyncio
from amqtt.broker import Broker # pip install amqtt paho-mqtt

config = {
    "listeners": {
        "default": {
            "type": "tcp",
            "bind": "127.0.0.1:1883",
            "max_connections": 50,
        }
    },
    "sys_interval": 10,
    "auth": {
        "allow-anonymous": True, # 익명 접속
    },
    # "auth": {
    #     "allow-anonymous": False,       # 익명 접속 차단
    #     "password-file": "passwd.txt",  # 계정 정보 파일
    # },
    # "plugins": [
    #     "auth_file"          # 파일 인증 플러그인 활성화
    # ]
}

async def start_broker():
    broker = Broker(config)
    await broker.start()
    print("MQTT Broker started on 127.0.0.1:1883")

    try:
        while True:
            await asyncio.sleep(1)
    except asyncio.CancelledError:
        await broker.shutdown()

if __name__ == "__main__":
    try:
        asyncio.run(start_broker())
    except KeyboardInterrupt:
        print("\nStopping MQTT Broker...")
