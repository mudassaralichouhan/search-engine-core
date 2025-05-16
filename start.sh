#!/bin/bash

# Create MongoDB data directory if it doesn't exist
mkdir -p /data/db

# Start MongoDB in the background with proper logging
mongod --bind_ip_all --logpath /var/log/mongodb.log --fork

# Wait for MongoDB to start
echo "Waiting for MongoDB to start..."
until mongosh --eval "print('MongoDB is up!')" > /dev/null 2>&1; do
    echo "Waiting for MongoDB to be ready..."
    sleep 2
done

echo "MongoDB is ready!"

# Start the server application
echo "Starting server application..."
./server &

# Keep the container running and show logs
tail -f /var/log/mongodb.log