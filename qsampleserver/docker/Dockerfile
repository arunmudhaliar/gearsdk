FROM ubuntu:23.10
LABEL key="qsampleserver"
EXPOSE 4000/udp
WORKDIR /app
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qhiredis/ ./qhiredis
COPY servercommon/ ./servercommon
COPY qutils/ ./qutils
COPY qsampleserver/ ./qsampleserver
COPY qserver/ ./qserver
COPY make_qsampleserver.sh ./
COPY run_qsampleserver.sh ./
COPY qsampleserver/certs/cert.crt ./qsampleserver/
COPY qsampleserver/certs/cert.key ./qsampleserver/
RUN apt-get update && apt-get install make -y
RUN apt-get install build-essential -y
RUN apt-get install libssl-dev -y
RUN apt install iproute2 -y
RUN apt-get install zlib1g-dev -y
RUN apt-get install libcurl4-openssl-dev -y
RUN sh make_qsampleserver.sh
CMD [ "sh", "run_qsampleserver.sh" ]