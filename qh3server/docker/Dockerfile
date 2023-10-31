FROM ubuntu:20.04
LABEL key="qhttp-server"
EXPOSE 4004/udp
WORKDIR /app
COPY qh3server/certs/cert.crt ./
COPY qh3server/certs/cert.key ./
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qh3server/ ./qh3server
COPY make_qhttp_server.sh ./
COPY run_qhttp_server.sh ./
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN apt install iproute2 -y
RUN sh make_qhttp_server.sh
CMD [ "sh", "run_qhttp_server.sh" ]