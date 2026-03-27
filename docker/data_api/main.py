from fastapi import FastAPI
import os
from pymongo import MongoClient, DESCENDING

# Import env variables
ap_prefix = os.getenv('AP_PREFIX')
db_uri = os.getenv('DB_URI', None)
db_name = os.getenv('DB_NAME', None)
col_name = os.getenv('DB_COL', None)
track_col_name = os.getenv('TRACK_COL', None)

mgc = MongoClient(db_uri)
try:
    print('Connecting MongoDB')
    db = mgc.get_database(db_name)
    db_col = db[col_name]
    track_col = db[track_col_name]
except Exception as err:
    print(err)
    
app = FastAPI()

@app.get('/')
def main():
    return {"status": "OK"}

@app.get("/wifi_ap/{ap_id}")
def get_ap(ap_id: int):
    ap_ssid = ap_prefix + '{:02d}'
    try:
        #ans = db_col.find_one({"AP":ap_ssid.format(ap_id)}, sort=[("_id", DESCENDING)])
        ans = db_col.find({"AP":ap_ssid.format(ap_id)})
        resp = [doc for doc in ans]
        print(resp)
        for doc in resp:
            doc.pop('_id')
    except:
        resp = {'STATUS': 'Too early, try later'}
    return resp

@app.get("/devices/{dev_id}")
def track_device(dev_id: str):
    ans = track_col.find_one({"ID": dev_id}, sort=[("_id", DESCENDING)])
    try:
        ans.pop('_id')
    except:
        ans = {"STATUS": "NO DEVICE"}
    return ans