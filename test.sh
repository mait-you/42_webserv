#!/bin/bash
# save as stress.sh
# chmod +x stress.sh

echo "Starting stress test..."
for i in $(seq 1 200); do
    curl -s -X GET http://localhost:8080/cgi-bin/debug.py > /dev/null &
done
wait
echo "Done"
