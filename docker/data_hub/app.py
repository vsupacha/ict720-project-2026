import paho.mqtt.client as mqtt
import os
import json
from pymongo import MongoClient

# Import env variables
mqtt_broker = os.getenv('MQTT_BROKER', 'broker.emqx.io')
data_topic = os.getenv('DATA_TOPIC', 'data_topic')
db_uri = os.getenv('DB_URI', None)
db_name = os.getenv('DB_NAME', None)

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    # Subscribing in on_connect()
    client.subscribe(data_topic)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    payload = json.loads(msg.payload)
    print(msg.topic + ': ' + str(payload))

mgc = MongoClient(db_uri)
try:
    print('Connecting MongoDB')
    db = mgc.get_database(db_name)
except Exception as err:
    print(err)

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message
print('Connecting MQTT broker')
mqttc.connect(mqtt_broker, 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks
mqttc.loop_forever()