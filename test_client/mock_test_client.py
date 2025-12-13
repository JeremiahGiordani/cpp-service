#!/usr/bin/env python3
"""
Mock Test Client for SAR ATR Service

This client sends FileLocation_uci messages to the AMQ broker to test
the SAR ATR service. It can send messages with real NITF file paths
from a directory, or send mock paths for communication testing.
"""

import json
import time
import random
import os
import yaml
from datetime import datetime, timezone
from stomp import Connection
import sys

class MockTestClient:
    def __init__(self, config_path='test_client_config.yaml'):
        """Initialize the mock client with configuration"""
        with open(config_path, 'r') as f:
            self.config = yaml.safe_load(f)
        
        self.broker_host = self.config['broker_host']
        self.broker_port = self.config['broker_port']
        self.nitf_directory = self.config.get('nitf_directory', '')
        self.send_interval = self.config.get('send_interval', 5)
        
        self.conn = None
        
    def connect(self):
        """Connect to the AMQ broker"""
        print(f"[INFO] Connecting to AMQ broker at {self.broker_host}:{self.broker_port}")
        try:
            self.conn = Connection([(self.broker_host, self.broker_port)])
            self.conn.connect(wait=True)
            print("[INFO] Connected to AMQ broker")
            return True
        except Exception as e:
            print(f"[ERROR] Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the broker"""
        if self.conn and self.conn.is_connected():
            self.conn.disconnect()
            print("[INFO] Disconnected from AMQ broker")
    
    def get_nitf_files(self):
        """Get list of NITF files from configured directory"""
        if not self.nitf_directory or not os.path.exists(self.nitf_directory):
            return []
        
        nitf_files = []
        for root, dirs, files in os.walk(self.nitf_directory):
            for file in files:
                if file.lower().endswith(('.ntf', '.nitf')):
                    nitf_files.append(os.path.join(root, file))
        
        return nitf_files
    
    def generate_file_location_message(self, file_path):
        """Generate a FileLocation_uci message"""
        timestamp = datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%S.%f')[:-3] + 'Z'
        
        message = {
            "FileLocation": {
                "MessageData": {
                    "LocationAndStatus": {
                        "Location": {
                            "Network": {
                                "Address": file_path
                            }
                        }
                    }
                }
            }
        }
        
        return json.dumps(message)
    
    def send_message(self):
        """Send a FileLocation_uci message"""
        if not self.conn or not self.conn.is_connected():
            print("[ERROR] Not connected to broker")
            return False
        
        # Get NITF files or use mock path
        nitf_files = self.get_nitf_files()
        
        if nitf_files:
            file_path = random.choice(nitf_files)
            print(f"[INFO] Selected NITF file: {file_path}")
        else:
            # Generate mock path for testing
            file_path = f"/mock/data/test_image_{random.randint(1000, 9999)}.nitf"
            print(f"[INFO] No NITF files found, using mock path: {file_path}")
        
        try:
            message = self.generate_file_location_message(file_path)
            self.conn.send(destination='/topic/FileLocation_uci', 
                          body=message,
                          headers={'content-type': 'application/json'})
            print(f"[INFO] Sent FileLocation_uci message for: {file_path}")
            return True
        except Exception as e:
            print(f"[ERROR] Failed to send message: {e}")
            return False
    
    def run_continuous(self):
        """Run continuously, sending messages at intervals"""
        print(f"[INFO] Starting continuous mode, sending every {self.send_interval} seconds")
        print("[INFO] Press Ctrl+C to stop")
        
        try:
            while True:
                self.send_message()
                time.sleep(self.send_interval)
        except KeyboardInterrupt:
            print("\n[INFO] Stopping continuous mode")
    
    def run_once(self):
        """Send a single message and exit"""
        print("[INFO] Sending single message")
        self.send_message()

def main():
    """Main entry point"""
    config_path = 'test_client_config.yaml'
    
    if len(sys.argv) > 1:
        config_path = sys.argv[1]
    
    client = MockTestClient(config_path)
    
    if not client.connect():
        return 1
    
    try:
        # Check for --once flag
        if '--once' in sys.argv:
            client.run_once()
        else:
            client.run_continuous()
    finally:
        client.disconnect()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
