FROM ubuntu:latest

RUN apt-get update
RUN apt-get install -y gcc make lld