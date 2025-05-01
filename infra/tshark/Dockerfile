FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    tshark \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /captures

ENTRYPOINT ["tshark"]
