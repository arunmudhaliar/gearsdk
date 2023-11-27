FROM ubuntu:20.04
LABEL key="qstats-crawler"
WORKDIR /app
COPY common ./common
COPY networkcommon/ ./networkcommon
COPY qstats-crawler/ ./qstats-crawler
COPY make_qstats-crawler.sh ./
COPY run_qstats-crawler.sh ./
RUN apt-get update && apt-get install make
RUN apt-get install build-essential -y
RUN sh make_qstats-crawler.sh
CMD [ "sh", "run_qstats-crawler.sh" ]