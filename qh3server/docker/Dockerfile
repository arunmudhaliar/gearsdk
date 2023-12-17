FROM ubuntu:23.10
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
RUN apt install libsnappy-dev -y
RUN apt-get install libzstd-dev -y
RUN apt install iproute2 -y
RUN sh make_qh3server.sh
# CMD [ "sh", "run_qh3server.sh" ]

# ------- qstats-crawler -------
# ------- qstats-crawler -------
# ------- qstats-crawler -------
COPY qstats-crawler/ ./qstats-crawler
COPY make_qstats-crawler.sh ./
COPY run_qstats-crawler.sh ./
COPY make_postgresql.sh ./
RUN apt-get install wget -y
RUN apt-get install libreadline-dev -y
RUN apt-get install zlib1g-dev -y
RUN wget https://ftp.postgresql.org/pub/source/v15.5/postgresql-15.5.tar.gz
RUN tar -xvzf postgresql-15.5.tar.gz
RUN pwd
RUN sh make_postgresql.sh
RUN pwd
RUN sh make_qstats-crawler.sh

RUN chmod 755 ./run_qh3server.sh
RUN chmod 755 ./run_qstats-crawler.sh
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
RUN apt-get install -y supervisor
RUN mkdir -p /var/log/supervisor
CMD ["/usr/bin/supervisord","-n"]