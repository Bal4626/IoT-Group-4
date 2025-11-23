# cloud_server.py
from flask import Flask, request
import sqlite3, datetime

app = Flask(__name__)

def get_db():
    conn = sqlite3.connect("anchors.db")
    conn.execute("""CREATE TABLE IF NOT EXISTS readings (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        ts TEXT,
        anchor_id TEXT,
        device_id TEXT,
        rssi INTEGER
    )""")
    return conn

@app.post("/upload")
def upload():
    data = request.json
    conn = get_db()
    conn.execute(
        "INSERT INTO readings (ts, anchor_id, device_id, rssi) VALUES (?, ?, ?, ?)",
        (data["timestamp"], data["anchor_id"], data["device_id"], data["rssi"])
    )
    conn.commit()
    conn.close()
    return {"status": "ok"}

app.run(host="0.0.0.0", port=5000)
