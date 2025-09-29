#!/bin/bash

### Script for AWS CodeDeploy to execute pre-install ###
set -e

cd $(dirname $0)/.. || exit 1

echo "Stopping running containers..."
docker compose -f comms/docker-compose.yml down

echo "Cleaning up old images..."
docker image prune -f

echo "Containers stopped and cleanup done."
