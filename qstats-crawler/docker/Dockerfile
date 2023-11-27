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
RUN rm -r /app/qstats-crawler/libs/linux
RUN mkdir /app/qstats-crawler/libs/linux
RUN cp -a /app/postgresql-15.5/build/lib/. /app/qstats-crawler/libs/linux
RUN ls /app/qstats-crawler/libs/linux/
RUN sh make_qstats-crawler.sh
CMD [ "sh", "run_qstats-crawler.sh" ]