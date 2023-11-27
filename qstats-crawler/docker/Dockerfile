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
RUN apt-get install libssl-dev -y
RUN apt-get install libsasl2-dev -y
RUN apt-get install libzstd-dev -y
RUN apt-get install wget -y
RUN apt-get install libreadline-dev -y
RUN apt-get install zlib1g-dev -y
RUN wget https://ftp.postgresql.org/pub/source/v15.5/postgresql-15.5.tar.gz
RUN tar -xvzf postgresql-15.5.tar.gz
RUN pwd
RUN mkdir /app/postgresql-15.5/build
RUN ls
RUN /app/postgresql-15.5/configure --prefix=/app/postgresql-15.5/build
RUN make install
RUN sh make_qstats-crawler.sh
CMD [ "sh", "run_qstats-crawler.sh" ]