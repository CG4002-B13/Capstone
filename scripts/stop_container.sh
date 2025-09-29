#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

cd /home/ubuntu/Capstone || exit 1

echo "Stopping running containers..."
docker compose down

echo "Cleaning up old images..."
docker image prune -f

echo "Containers stopped and cleanup done."
