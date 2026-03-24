from fastapi import FastAPI
import os
from pymongo import MongoClient

# Import env variables
db_name = os.getenv('DB_NAME', 'demo_db')
ap_prefix = os.getenv('AP_PREFIX')
db_uri = os.getenv('DB_URI', None)
db_name = os.getenv('DB_NAME', None)

mgc = MongoClient(db_uri)
try:
    print('Connecting MongoDB')
    db = mgc.get_database(db_name)
except Exception as err:
    print(err)
    
app = FastAPI()

@app.get('/')
def main():
    return {"status": "OK"}

@app.get("/wifi_ap/{ap_id}")
def get_ap(ap_id: int):
    ap_ssid = ap_prefix + '{:02d}'
    return {"AP": ap_ssid.format(ap_id)}