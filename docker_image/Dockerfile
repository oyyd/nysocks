FROM node:8

MAINTAINER oyyd <oyydoibh@gmail.com>

# use unsafe-perm to prevent some access issues
RUN npm config set user 0
RUN npm config set unsafe-perm true

RUN npm i -g nysocks
RUN mkdir -p nysocks
COPY config.json nysocks/

# nysocks needs a port for each socket.
EXPOSE 1-65535

CMD nysocks server -c nysocks/config.json -d restart
