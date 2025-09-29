#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

cd /home/ubuntu/app/comms || exit 1

echo "Pulling latest images from ECR..."
sudo docker compose -f docker-compose.yml pull

echo "Starting containers..."
sudo docker compose -f docker-compose.yml up -d

echo "Deployment finished successfully."
