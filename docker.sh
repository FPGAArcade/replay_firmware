#!/bin/bash
set -ex
docker rm -f replay
docker rmi -f replay
docker build -t replay .
docker run -v $PWD:/host -w /host -e TERM=$TERM --name replay -it replay /bin/bash -c "$*"

# ex.
# ./docker.sh ./make_all.sh
