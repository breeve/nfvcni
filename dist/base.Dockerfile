FROM ubuntu:24.04

RUN apt-get update

# go
RUN apt-get install -y wget && \
    wget https://go.dev/dl/go1.26.0.linux-amd64.tar.gz && \
    tar -C /usr/local -xzf go1.26.0.linux-amd64.tar.gz && \
    rm go1.26.0.linux-amd64.tar.gz && \
    ln -s /usr/local/go/bin/go /usr/bin/go

ENV PATH=$PATH:/usr/local/go/bin

# tools
RUN apt-get install -y \
    make \
    vim \
    git \
    jq \
    psmisc

# network tools
RUN apt-get install -y \
    iproute2 \
    net-tools \
    tcpdump \
    iputils-ping \
    curl \
    ethtool \
    iperf3
