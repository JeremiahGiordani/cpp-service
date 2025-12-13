#!/bin/bash
# Quick start script for SAR ATR Service

set -e

echo "========================================="
echo "SAR ATR Service - Quick Start"
echo "========================================="
echo ""

# Check for Docker
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed"
    echo "Please install Docker: https://docs.docker.com/get-docker/"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo "Error: Docker Compose is not installed"
    echo "Please install Docker Compose: https://docs.docker.com/compose/install/"
    exit 1
fi

echo "✓ Docker and Docker Compose found"
echo ""

# Build containers
echo "Building containers..."
docker-compose build

echo ""
echo "✓ Containers built successfully"
echo ""

# Start services
echo "Starting services..."
docker-compose up -d

echo ""
echo "✓ Services started"
echo ""

# Wait for services to be ready
echo "Waiting for services to initialize..."
sleep 5

# Show status
echo ""
echo "========================================="
echo "Service Status"
echo "========================================="
docker-compose ps
echo ""

# Show logs
echo "========================================="
echo "Service Logs (last 20 lines)"
echo "========================================="
docker-compose logs --tail=20 sar_atr_service
echo ""

echo "========================================="
echo "Quick Start Complete!"
echo "========================================="
echo ""
echo "The service is now running!"
echo ""
echo "Useful commands:"
echo "  View live logs:     docker-compose logs -f"
echo "  Stop services:      docker-compose down"
echo "  Restart services:   docker-compose restart"
echo ""
echo "ActiveMQ Web Console: http://localhost:8161"
echo "  Username: admin"
echo "  Password: admin"
echo ""
echo "To test the service, run the test client:"
echo "  cd test_client"
echo "  pip install -r requirements.txt"
echo "  python mock_test_client.py --once"
echo ""
