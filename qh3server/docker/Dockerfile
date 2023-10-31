FROM ubuntu:20.04
LABEL key="qh3server"
EXPOSE 4004/udp
WORKDIR /app
COPY qh3server/certs/cert.crt ./
COPY qh3server/certs/cert.key ./
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qh3server/ ./qh3server
COPY make_qh3server.sh ./
COPY run_qh3server.sh ./
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN apt install iproute2 -y
RUN sh make_qh3server.sh
CMD [ "sh", "run_qh3server.sh" ]