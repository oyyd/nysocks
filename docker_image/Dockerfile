FROM node:8

MAINTAINER oyyd <oyydoibh@gmail.com>

RUN npm i -g nysocks
RUN mkdir -p nysocks
COPY config.json nysocks/

# nysocks needs a port for each socket.
EXPOSE 1-65535

CMD nysocks server -c nysocks/config.json -d restart
