FROM ubuntu:23.10
LABEL key="qh3client"
EXPOSE 4004/udp
EXPOSE 4010/udp
EXPOSE 4011/udp
EXPOSE 5100-65535/udp
WORKDIR /app
COPY common ./common
COPY networkcommon/ ./networkcommon
# COPY qh3server/ ./qh3server
COPY qh3client/ ./qh3client
# COPY qhiredis/ ./qhiredis
# COPY servercommon/ ./servercommon
# COPY qzookeeper/ ./qzookeeper
COPY make_qh3client.sh ./
COPY run_qh3client.sh ./
# COPY qh3server/certs/cert.crt ./qh3server/
# COPY qh3server/certs/cert.key ./qh3server/
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN apt-get install libssl-dev -y
RUN apt-get install libsasl2-dev -y
RUN apt install libsnappy-dev -y
RUN apt-get install libzstd-dev -y
RUN apt install iproute2 -y
RUN sh make_qh3client.sh
RUN chmod 755 ./run_qh3client.sh

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y tshark
CMD [ "sh", "run_qh3client.sh" ]