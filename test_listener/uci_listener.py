#!/usr/bin/env python3
"""
UCI Message Listener

This script listens to Entity_uci and AtrProcessingResult_uci messages
from the AMQ broker and pretty prints them for monitoring and debugging.
"""

import json
import yaml
from stomp import Connection, ConnectionListener
import sys
import time
from datetime import datetime

class UCIMessageListener(ConnectionListener):
    """Listener for UCI messages"""
    
    def __init__(self):
        self.message_count = 0
    
    def on_error(self, frame):
        """Handle error frames"""
        print(f'\n[ERROR] Received an error: {frame.body}')
    
    def on_message(self, frame):
        """Handle incoming messages"""
        self.message_count += 1
        
        # Get destination (topic name)
        destination = frame.headers.get('destination', 'Unknown')
        topic = destination.split('/')[-1] if '/' in destination else destination
        
        # Get timestamp
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        
        print('\n' + '='*80)
        print(f'[{timestamp}] Message #{self.message_count} on topic: {topic}')
        print('='*80)
        
        try:
            # Parse and pretty print JSON
            message_data = json.loads(frame.body)
            print(json.dumps(message_data, indent=2, sort_keys=False))
        except json.JSONDecodeError:
            # If not JSON, print as-is
            print(frame.body)
        
        print('='*80)
        print()

class UCIListener:
    def __init__(self, config_path='listener_config.yaml'):
        """Initialize the UCI listener with configuration"""
        with open(config_path, 'r') as f:
            self.config = yaml.safe_load(f)
        
        self.broker_host = self.config['broker_host']
        self.broker_port = self.config['broker_port']
        self.topics = self.config.get('topics', ['Entity_uci', 'AtrProcessingResult_uci'])
        
        self.conn = None
        self.listener = UCIMessageListener()
    
    def connect(self):
        """Connect to the AMQ broker"""
        print(f"[INFO] Connecting to AMQ broker at {self.broker_host}:{self.broker_port}")
        try:
            self.conn = Connection([(self.broker_host, self.broker_port)])
            self.conn.set_listener('uci_listener', self.listener)
            self.conn.connect(wait=True)
            print("[INFO] Connected to AMQ broker")
            return True
        except Exception as e:
            print(f"[ERROR] Failed to connect: {e}")
            return False
    
    def subscribe(self):
        """Subscribe to configured topics"""
        for topic in self.topics:
            try:
                self.conn.subscribe(destination=f'/topic/{topic}', id=f'sub-{topic}', ack='auto')
                print(f"[INFO] Subscribed to topic: {topic}")
            except Exception as e:
                print(f"[ERROR] Failed to subscribe to {topic}: {e}")
    
    def disconnect(self):
        """Disconnect from the broker"""
        if self.conn and self.conn.is_connected():
            self.conn.disconnect()
            print("\n[INFO] Disconnected from AMQ broker")
    
    def run(self):
        """Run the listener continuously"""
        print("="*80)
        print("UCI Message Listener")
        print("="*80)
        print(f"Listening to topics: {', '.join(self.topics)}")
        print("Press Ctrl+C to stop")
        print("="*80)
        print()
        
        if not self.connect():
            return 1
        
        self.subscribe()
        
        try:
            # Keep the listener running
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print("\n[INFO] Stopping listener...")
        finally:
            self.disconnect()
        
        print(f"\n[INFO] Total messages received: {self.listener.message_count}")
        return 0

def main():
    """Main entry point"""
    config_path = 'listener_config.yaml'
    
    if len(sys.argv) > 1:
        config_path = sys.argv[1]
    
    listener = UCIListener(config_path)
    return listener.run()

if __name__ == '__main__':
    sys.exit(main())
