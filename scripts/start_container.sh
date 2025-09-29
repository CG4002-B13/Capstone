#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

AWS_REGION="ap-southeast-1"
AWS_ACCOUNT_ID="828171332858"
ECR_REGISTRY="${AWS_ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com"

cd /home/ubuntu/app/comms || exit 1

echo "Logging into AWS ECR..."
aws ecr get-login-password --region ${AWS_REGION} | sudo docker login --username AWS --password-stdin ${ECR_REGISTRY}
echo "Pulling latest images from ECR..."
sudo docker compose -f docker-compose.yml pull

echo "Starting containers..."
sudo docker compose -f docker-compose.yml up -d

echo "Deployment finished successfully."
