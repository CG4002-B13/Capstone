#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

cd /home/ubuntu/Capstone || exit 1

echo "Pulling latest images from ECR..."
docker compose pull

echo "Starting containers..."
docker compose up -d

echo "Deployment finished successfully."
