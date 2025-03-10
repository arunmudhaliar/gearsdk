# Use a Debian-based Node.js image
FROM node:20-bullseye
LABEL key="server_app_ts"

# for user server
EXPOSE 4004/udp
EXPOSE 4010/udp
EXPOSE 4011/udp
EXPOSE 5100-5116/udp

# for game server
EXPOSE 4000/udp

# Set working directory
WORKDIR /app

# Copy files and folders
COPY package.json ./
COPY package-lock.json ./
COPY tsconfig.json ./
COPY src ./src
COPY configs ./configs
COPY cert.crt ./
COPY cert.key ./
COPY serverconfig.dev.inf ./
COPY serverconfig.rel.inf ./

# Install required system dependencies
RUN apt-get update && apt-get install -y libsasl2-dev libssl-dev libsnappy-dev libunwind-dev build-essential

# Add Debian backports to get OpenSSL 3.x
RUN echo "deb http://deb.debian.org/debian bookworm main" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y libssl3 gcc-11 g++-11

# Install dependencies
RUN npm install

# Build TypeScript project
RUN npm run build

# Start the application
CMD ["npm", "run", "start"]


# docker run -d \
#   --name server_app \
#   -p 4004:4004/udp \
#   -p 4010:4010/udp \
#   -p 4011:4011/udp \
#   -p 5100-5116:5100-5116/udp \
#   -p 4000:4000/udp \
#   server_app_ts_proj

