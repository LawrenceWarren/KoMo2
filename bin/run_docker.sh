#!/bin/sh

docker run --name kcmd --platform linux/amd64 -t --detach ubuntu sleep inf
docker exec -it kcmd apt -y update > /dev/null
docker exec -it kcmd apt install -y gcc-multilib > /dev/null
docker cp aasm kcmd:/tmp/
docker cp mnemonics kcmd:/tmp/
