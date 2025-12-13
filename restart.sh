#!/bin/bash
# Restart script with rebuild

set -e

echo "========================================="
echo "Rebuilding and Restarting Services"
echo "========================================="
echo ""

# Stop existing containers
echo "Stopping existing containers..."
docker-compose down

echo ""
echo "Rebuilding service..."
docker-compose build sar_atr_service

echo ""
echo "Starting services..."
docker-compose up

