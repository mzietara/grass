#! /bin/bash

# USAGE
# run ./run.sh [RCOST_INPUT] [RCOST_OUTPUT]
# ex: ./run.sh Site1V2 dorian8

# Edit these values
datadir=/home/mark/newgrass/data
permFile=test3/PERMANENT
startCoordinates="1400340.87, 11830661.95" 
version=1.0


set -e

if [ -z "$1" ]
  then
    echo "first arg is rcost input"
    exit 1
fi
if [ -z "$2" ]
  then
    echo "second arg is rcost output"
    exit 1
fi

rcostInput=$1
rcostOutput=$2
imageName="mzietara/crossingsurvivalgis:$version"
docker pull $imageName

docker run --rm --user=$(id -u):$(id -g) --volume $datadir:/data \
  --env HOME=/data/ $imageName grass $permFile --exec r.cost input=$rcostInput \
  start_coordinates="$startCoordinates" output=$rcostOutput
