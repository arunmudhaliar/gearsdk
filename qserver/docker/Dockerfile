FROM ubuntu:20.04
LABEL key="qserver"
EXPOSE 4000/udp
WORKDIR /app
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qserver/ ./qserver
COPY make_qserver.sh ./
COPY run_qserver.sh ./
COPY qserver/certs/cert.crt ./qserver/
COPY qserver/certs/cert.key ./qserver/
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN apt install iproute2 -y
RUN sh make_qserver.sh
CMD [ "sh", "run_qserver.sh" ]