# anchor_scan.py
import asyncio, datetime, csv
from bleak import BleakScanner
from awscrt import mqtt
from awsiot import mqtt_connection_builder
import json


ANCHOR_ID = "rpi_anchor_1"
rssi_history = {}  # device.address -> list of recent rssi values

async def main():
    with open("scan_log.csv", "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "wristband_id", "anchor_id",
                         "confidence", "anchor_x", "anchor_y", "anchor_z",
                         "distance"])

        def callback(device, adv_data):
            name = adv_data.local_name or ""
            if not name.startswith("WB"):
                return

            ts = datetime.datetime.utcnow().isoformat() + "Z"
            rssi = adv_data.rssi
            addr = device.address

            # keep last 5 RSSI samples per wristband
            hist = rssi_history.get(addr, [])
            hist.append(rssi)
            if len(hist) > 5:
                hist.pop(0)
            rssi_history[addr] = hist

            # ---- confidence pieces ----
            N_anchors = 1
            Ca = min(1.0, N_anchors / 3.0)

            N_samples = len(hist)
            mean = sum(hist) / N_samples
            var = sum((x - mean) ** 2 for x in hist) / max(1, N_samples - 1)
            Cv = 1.0 / (1.0 + 0.05 * var)

            Cs = min(1.0, N_samples / 5.0)

            confidence = 0.4 * Ca + 0.4 * Cv + 0.2 * Cs

            payload = {
                "wristband_id": name,              # "WB1"
                "anchor_id": ANCHOR_ID,            # "rpi_anchor_1"
                "confidence": confidence,
                "anchor_pos": {"x": 0.0, "y": 0.0, "z": 0.0},
                "timestamp": ts,
                "distance": rssi                   # still in dBm
            }

            #print(payload)
            mqtt_connection.publish(
                topic=TOPIC,
                payload=json.dumps(payload),
                qos=mqtt.QoS.AT_LEAST_ONCE
            )
            print("Published:", payload)


            # # also log to CSV
            # writer.writerow([
            #     ts, name, ANCHOR_ID,
            #     confidence, 0.0, 0.0, 0.0,
            #     rssi
            # ])
            # f.flush()
        # --- AWS IoT MQTT SETUP ---
        ENDPOINT = "a16ii7k74cp6ku-ats.iot.ap-southeast-1.amazonaws.com"   # example: a123456789-ats.iot.ap-southeast-1.amazonaws.com
        CLIENT_ID = "anchor_rpi_1"
        TOPIC = "iot/anchors/data"

        mqtt_connection = mqtt_connection_builder.mtls_from_path(
            endpoint=ENDPOINT,
            cert_filepath="f35bd20667d1d2e3df2dc0e4f9c723c536923f2ac7510aecc6c6cad8703e186b.crt",
            pri_key_filepath="f35bd20667d1d2e3df2dc0e4f9c723c536923f2ac7510aecc6c6cad8703e186b.key",
            ca_filepath="AmazonRootCA1.pem",
            client_id=CLIENT_ID,
            clean_session=False,
            keep_alive_secs=30
        )

        print("Connecting to AWS IoT...")
        connect_future = mqtt_connection.connect()
        connect_future.result()
        print("Connected!")

        

        scanner = BleakScanner(callback)
        await scanner.start()
        await asyncio.sleep(999999)  # run "forever"
        await scanner.stop()

asyncio.run(main())


