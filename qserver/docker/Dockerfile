FROM ubuntu:20.04
LABEL key="qserver"
EXPOSE 4000/udp
WORKDIR /app
RUN mkdir -p /app/certs
COPY qserver-app /app
COPY certs/cert.crt /app/certs
COPY certs/cert.key /app/certs
CMD ["./qserver-app"]