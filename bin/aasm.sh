#!/bin/sh
# for mac

container_name=kcmd

docker cp $1/$2.s $container_name:/tmp/
docker exec -it $container_name /tmp/aasm -lk /tmp/$2.kmd /tmp/$2.s
docker cp $container_name:/tmp/$2.kmd $1/

