
import paho.mqtt.client as mqtt
import time
import threading
import sys

# Broker Configuration
BROKER_ADDRESS = "192.168.1.21"
PORT = 1883
TOPIC_SUB = "irrigation/#"
TOPIC_PUB = "irrigation/command"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"✅ Connected to MQTT Broker at {BROKER_ADDRESS}")
        client.subscribe(TOPIC_SUB)
        print(f"📡 Subscribed to topic: {TOPIC_SUB}")
    else:
        print(f"❌ Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        print(f"\n📩 Received [{msg.topic}]: {payload}")
        print("> ", end="", flush=True) # Restore prompt
    except Exception as e:
        print(f"\nError decoding message: {e}")

def input_loop(client):
    print("\n⌨️  Enter commands to publish to 'irrigation/command' (or 'q' to quit):")
    time.sleep(1) 
    print("> ", end="", flush=True)
    while True:
        try:
            cmd = input()
            if cmd.lower() == 'q':
                print("Exiting...")
                client.disconnect()
                break
            if cmd:
                client.publish(TOPIC_PUB, cmd)
                print(f"📤 Sent: {cmd}")
                print("> ", end="", flush=True)
        except EOFError:
            break

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"🔄 Connecting to {BROKER_ADDRESS}...")
    try:
        client.connect(BROKER_ADDRESS, PORT, 60)
    except Exception as e:
        print(f"❌ Connection failed: {e}")
        return

    # Start the network loop in a background thread
    client.loop_start()

    # Run the input loop in the main thread
    try:
        input_loop(client)
    except KeyboardInterrupt:
        print("\nDisconnecting...")
    
    client.loop_stop()
    client.disconnect()

if __name__ == "__main__":
    main()
