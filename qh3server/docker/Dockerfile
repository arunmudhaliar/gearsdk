FROM ubuntu:20.04
LABEL key="qh3server"
EXPOSE 4004/udp
WORKDIR /app
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qh3server/ ./qh3server
COPY qhiredis/ ./qhiredis
COPY servercommon/ ./servercommon
COPY make_qh3server.sh ./
COPY run_qh3server.sh ./
COPY qh3server/certs/cert.crt ./qh3server/
COPY qh3server/certs/cert.key ./qh3server/
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN apt-get install libssl-dev -y
RUN apt-get install libsasl2-dev -y
# RUN apt install libmongoc-dev -y
RUN apt install libsnappy-dev -y
RUN apt-get install libzstd-dev -y
RUN apt install iproute2 -y
RUN sh make_qh3server.sh
CMD [ "sh", "run_qh3server.sh" ]