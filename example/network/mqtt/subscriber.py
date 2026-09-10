import paho.mqtt.client as mqtt # pip install amqtt paho-mqtt

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Connected to MQTT Broker successfully.")
        client.subscribe("test/topic")
        print("Subscribed to topic: test/topic")
    else:
        print(f"Failed to connect. Return code: {reason_code}")

def on_message(client, userdata, msg):
    print(f"Received message on [{msg.topic}]: {msg.payload.decode('utf-8')}")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

print("Connecting to broker at 127.0.0.1:1883...")
client.connect("127.0.0.1", 1883)

try:
    client.loop_forever()
except KeyboardInterrupt:
    print("\nDisconnecting subscriber...")
    client.disconnect()
