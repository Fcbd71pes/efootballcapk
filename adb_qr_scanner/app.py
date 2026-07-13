import logging
import subprocess
import threading
import io
import time
import base64
import os
from random import randint
from shutil import which
from flask import Flask, render_template, jsonify

import qrcode
from zeroconf import IPVersion, ServiceBrowser, ServiceInfo, ServiceListener, Zeroconf

app = Flask(__name__)

TYPE = "_adb-tls-pairing._tcp.local."
NAME = f"studio-{randint(100, 999)}"
PASSWORD = randint(100000, 999999)
FORMAT_QR = "WIFI:T:ADB;S:{name};P:{password};;"
SUCCESS_MSG = "Successfully paired"

paired_status = {"paired": False, "message": ""}
zeroconf = None
browser = None

def get_adb_path():
    # Check if in PATH
    adb_path = which("adb")
    if adb_path:
        return adb_path
    
    # Check standard Windows location
    local_app_data = os.environ.get("LOCALAPPDATA", "")
    windows_path = os.path.join(local_app_data, "Android", "Sdk", "platform-tools", "adb.exe")
    if os.path.exists(windows_path):
        return windows_path
        
    return None

class ADBListener(ServiceListener):
    def remove_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        pass

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        info = zc.get_service_info(type_, name)
        if info:
            self.pair(info)

    def pair(self, info: ServiceInfo) -> None:
        global paired_status
        try:
            adb_cmd = get_adb_path()
            if not adb_cmd:
                logging.error("ADB not found!")
                return

            ip_address = info.ip_addresses_by_version(IPVersion.All)[0].exploded
            cmd = [adb_cmd, "pair", f"{ip_address}:{info.port}", str(PASSWORD)]
            logging.info(f"Executing: {' '.join(cmd)}")
            
            process = subprocess.run(cmd, capture_output=True, text=True)
            stdout = process.stdout
            
            if process.returncode != 0:
                logging.error(f"Pairing failed: {process.stderr}")
                paired_status = {"paired": False, "message": "Pairing failed"}
                return

            if SUCCESS_MSG in stdout:
                logging.info(f"Successfully paired with {ip_address}:{info.port}")
                paired_status = {"paired": True, "message": f"Successfully paired with {ip_address}"}
        except Exception as e:
            logging.error(f"Error during pairing: {e}")

    def update_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        pass

def start_zeroconf():
    global zeroconf, browser
    if not get_adb_path():
        logging.error("ADB not found in PATH or standard directory!")
        return
    
    zeroconf = Zeroconf()
    listener = ADBListener()
    browser = ServiceBrowser(zeroconf, TYPE, listener)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/qr")
def get_qr():
    qr_data = FORMAT_QR.format(name=NAME, password=PASSWORD)
    qr = qrcode.QRCode(box_size=10, border=4)
    qr.add_data(qr_data)
    qr.make(fit=True)
    
    img = qr.make_image(fill_color="black", back_color="white")
    buf = io.BytesIO()
    img.save(buf, format="PNG")
    img_b64 = base64.b64encode(buf.getvalue()).decode("utf-8")
    
    return jsonify({"qr_image": f"data:image/png;base64,{img_b64}"})

@app.route("/status")
def status():
    return jsonify(paired_status)

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    
    # Start mDNS discovery in background
    threading.Thread(target=start_zeroconf, daemon=True).start()
    
    # Run flask
    app.run(host="0.0.0.0", port=5000, debug=False)
