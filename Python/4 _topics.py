import serial
import paho.mqtt.client as mqtt
import re

# Configure serial port
SERIAL_PORT = "COM9"  # Replace with the correct COM port for your ESP32
BAUD_RATE = 115200

# MQTT Configuration
MQTT_BROKER = "broker.mqtt.cool"  # Replace with your broker
MQTT_PORT = 1883
MQTT_TOPIC_TEMP_SLAVE1 = "esp/now/slave1/temp"
MQTT_TOPIC_HUM_SLAVE1 = "esp/now/slave1/humidity"
MQTT_TOPIC_TEMP_SLAVE2 = "esp/now/slave2/temp"
MQTT_TOPIC_HUM_SLAVE2 = "esp/now/slave2/humidity"

# Initialize serial and MQTT client
ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
mqtt_client = mqtt.Client()

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
    else:
        print(f"Failed to connect, return code {rc}")

mqtt_client.on_connect = on_connect
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)

mqtt_client.loop_start()

# Regular expression to match data format
data_regex = r"Received Data from Slave(\d+): Temp = ([\d.]+)°C, Humidity = ([\d.]+)%, Alert = (.*)"

try:
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            print(f"Received: {line}")

            # Match the data using regex
            match = re.match(data_regex, line)

            if match:
                slave_id = match.group(1)  # Slave number (1 or 2)
                temperature = float(match.group(2))  # Temperature value
                humidity = float(match.group(3))  # Humidity value

                # Depending on the slave id, publish temperature and humidity to corresponding topics
                if slave_id == "1":
                    mqtt_client.publish(MQTT_TOPIC_TEMP_SLAVE1, temperature)
                    print(f"Published to {MQTT_TOPIC_TEMP_SLAVE1}: {temperature}")
                    
                    mqtt_client.publish(MQTT_TOPIC_HUM_SLAVE1, humidity)
                    print(f"Published to {MQTT_TOPIC_HUM_SLAVE1}: {humidity}")
                elif slave_id == "2":
                    mqtt_client.publish(MQTT_TOPIC_TEMP_SLAVE2, temperature)
                    print(f"Published to {MQTT_TOPIC_TEMP_SLAVE2}: {temperature}")
                    
                    mqtt_client.publish(MQTT_TOPIC_HUM_SLAVE2, humidity)
                    print(f"Published to {MQTT_TOPIC_HUM_SLAVE2}: {humidity}")
                else:
                    print(f"Invalid slave ID: {slave_id}")
            else:
                print(f"Invalid data format: {line}")

except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
