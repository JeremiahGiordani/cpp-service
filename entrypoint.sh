#!/bin/bash
set -e

echo "Waiting for ActiveMQ WebSocket to be ready..."
MAX_RETRIES=30
RETRY_COUNT=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if nc -z activemq 61614; then
        echo "ActiveMQ WebSocket port 61614 is open!"
        sleep 3
        echo "Starting SAR ATR Service..."
        exec /usr/local/bin/sar_atr_service /etc/sar_atr/service_config.yaml
    fi

    RETRY_COUNT=$((RETRY_COUNT + 1))
    echo "Waiting for ActiveMQ... attempt $RETRY_COUNT/$MAX_RETRIES"
    sleep 2
done

echo "ERROR: ActiveMQ did not become ready in time"
exit 1