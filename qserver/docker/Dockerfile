FROM ubuntu:20.04
LABEL key="qserver"
EXPOSE 4000/udp
WORKDIR /app
COPY qserver/certs/cert.crt ./
COPY qserver/certs/cert.key ./
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qserver/ ./qserver
COPY make_qserver.sh ./
COPY run_qserver.sh ./
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN sh make_qserver.sh
CMD [ "sh", "run_qserver.sh" ]