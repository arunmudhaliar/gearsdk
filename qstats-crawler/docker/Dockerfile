FROM ubuntu:20.04
LABEL key="qh3server"
EXPOSE 4004/udp
WORKDIR /app
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qh3server/ ./qh3server
COPY make_qstats-crawler.sh ./
COPY run_qstats-crawler.sh ./
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN sh make_qstats-crawler.sh
CMD [ "sh", "run_qstats-crawler.sh" ]