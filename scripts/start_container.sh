#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

cd /home/ubuntu/app/comms || exit 1

echo "Logging into AWS ECR..."
aws ecr get-login-password --region ap-southeast-1 | docker login --username AWS --password-stdin 828171332858.dkr.ecr.ap-southeast-1.amazonaws.com

echo "Pulling latest images from ECR..."
sudo docker compose -f docker-compose.yml pull

echo "Starting containers..."
sudo docker compose -f docker-compose.yml up -d

echo "Deployment finished successfully."
