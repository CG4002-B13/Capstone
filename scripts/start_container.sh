#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

cd $(dirname $0)/.. || exit 1

echo "Pulling latest images from ECR..."
docker compose pull

echo "Starting containers..."
docker compose -f comms/middleware/docker-compose.yml up -d

echo "Deployment finished successfully."
