import time
import paho.mqtt.client as mqtt # pip install amqtt paho-mqtt

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

print("Connecting to broker at 127.0.0.1:1883...")
# client.username_pw_set("admin_user", "secret_pass_1234")
client.connect("127.0.0.1", 1883)

# 백그라운드 네트워크 수발신 루프 시작
client.loop_start()

topic = "test/topic"
count = 1

try:
    while True:
        timestamp = time.strftime("%H:%M:%S")
        message = f"Hello from MQTT Publisher! (seq: {count})"

        result = client.publish(topic, message, qos=0)
        status = result[0]

        if status == 0:
            print(f"[{timestamp}] Published '{message}' to topic '{topic}'")
        else:
            print(f"[{timestamp}] Failed to send message to topic '{topic}'")

        count += 1
        time.sleep(10)

except KeyboardInterrupt:
    print("\nStopping publisher...")

finally:
    client.loop_stop()
    client.disconnect()
    print("Disconnected from broker.")


