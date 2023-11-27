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
RUN wget https://ftp.postgresql.org/pub/source/v15.5/postgresql-15.5.tar.gz
RUN tar -xvzf postgresql-15.5.tar.gz
RUN cd postgresql-15.5
RUN pwd
RUN mkdir build
RUN ls
RUN ./configure --prefix=/app/postgresql-15.5/build
RUN make install
RUN sh make_qstats-crawler.sh
CMD [ "sh", "run_qstats-crawler.sh" ]