#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

cd $(dirname $0)/.. || exit 1

echo "Stopping running containers..."
sudo docker compose -f comms/docker-compose.yml down

echo "Cleaning up old images..."
sudo docker image prune -f

echo "Containers stopped and cleanup done."
