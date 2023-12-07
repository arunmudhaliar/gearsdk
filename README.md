# gearsdk v0.1

Directories
-----------
|  directory ||
| ------------ | ------------ |
|common  | source codes used by both server and client  |
|networkcommon  |  again common codebase used by both server and client, but utilities associated with netwoking solutions. |
|qclient   [sdk]   | client sdk for statefull server-client communications using quic |
|qh3client [sdk]   | client sdk for stateless server-client communications using quic |
|qh3server [sdk]   | server sdk for stateless server-client communications using quic |
|qhiredis          | redis sdk wrapper [high speed in-memory db]|
|qserver   [sdk]   | server sdk for statefull server-client communications using quic|
|sandbox           |experimental code [only for reference purpose]|
|scripts           |jenkins sample script. (Note: qserver and qh3server contains more updated versions)|
|servercommon      | source code used only in server sdks. clients must not use these|
|docker-qh3server  | temporary directory while building for docker. |
|docker-qserver    |temporary directory while building for docker. |


Ubuntu build
---------------

     sudo apt install clang
     sudo apt-get install libc++-dev
     sudo apt-get install g++multilib

 - Use the `Makefile` in the respective directories to build individual sdks.

Building qserver for docker
--------------------------------
  - `sudo usermod -a -G docker jenkins`   (For setting user, one time setting.)

  - Please refer **jenkins_script.sh** file under qserver folder.

    - Go to 'qserver/docker' folder.
    - `sh ./prepare-docker-qserver-folder.sh`
            This will prepare a docker-server folder in root folder.
    - `docker build -t qserver-exp .`
    - `docker stop qserver-container`        [optional : if there is a previous docker running]
    - `docker rm --force qserver-container`   [optional : if there is a previous docker running]
    - `docker run --publish 4000:4000/udp --name qserver-container -d qserver-exp`

Install mongodb-community in docker
-----------------------------------
- https://www.mongodb.com/docs/manual/tutorial/install-mongodb-community-with-docker/

- `docker pull mongodb/mongodb-community-server`
- `docker run --publish 6006:27017/tcp --name mongo -d mongodb/mongodb-community-server:latest`
    Note : we are connecting mongo docker's host machine in 6006 port not its default port numer 27017.

Useful tips
-----------
- Response header added (mandatory for h3 response, now we are running in 6121 port. In-case we want to change we may need to refactor the code here.)

        Alternate-Protocol: quic:<QUIC server port>
        {
            .name = (uint8_t *) "Alternate-Protocol",
            .name_len = sizeof("Alternate-Protocol") - 1,
    
            .value = (uint8_t *) "quic:6121",
            .value_len = sizeof("quic:6121") - 1,
        },


- SPKI code generation cmd. Go to the cert folder and issue this command.


             cat cert.crt |
                  openssl x509 -inform pem -noout -outform pem -pubkey |
                  openssl pkey -pubin -inform pem -outform der |
                  openssl dgst -sha256 -binary |
                  openssl enc -base64
            [MCFtYhgL/+T4kkcV64TQTTAw0Q5Gq2360530xEr9lFs=]


- Chrome Canary : Good to test h3 response easily.

        

    /Applications/Google\ Chrome\ Canary.app/Contents/MacOS/Google\ Chrome\ Canary ----enable-quic --origin-to-force-quic-on=localhost:6121 https://localhost:6121/whoami --ignore-certificate-errors-spki-list=<SPKI>
        
    /Applications/Google\ Chrome\ Canary.app/Contents/MacOS/Google\ Chrome\ Canary ----enable-quic --origin-to-force-quic-on=localhost:6121 https://localhost:6121/ --ignore-certificate-errors-spki-list=MCFtYhgL/+T4kkcV64TQTTAw0Q5Gq2360530xEr9lFs=

** Make sure to modify the url and port before trying on your machine.

- Mongo
    https://www.mongodb.com/
    
    Built libs used in our codebase from this git repo
    mongo-c-driver
        git url  - https://github.com/mongodb/mongo-c-driver
    
 - Installation
        Please refer the official site for respective OS.
    
    To start/stop the service on MacOS if installed using brew
        - brew services start mongodb/brew/mongodb-community
        - brew services stop mongodb/brew/mongodb-community

- Redis
    Installation
        Please refer the official site for respective OS.

    Built libs used in our codebase from this git repo
    git https://github.com/redis/hiredis.git

- QUICHE
    QUIC protocol code base

    quiche built from this git repo
    https://github.com/cloudflare/quiche.git

- Connect to a docker container shell
    `sudo docker exec -it qh3server-container /bin/bash`

- Tailing the log file for dedbug purpose
    // tail last 100 lines.
    `tail -f -n 100 <logfile path>   [./qh3_logfile-0-0.log]`
	
	
	

Flamegraph using dtrace
-----------------------------------
https://carol-nichols.com/2017/04/20/rust-profiling-with-dtrace-on-osx/

```bash
# generate the stacks file
sudo dtrace -c './qh3server' -o ./$(date +"%Y-%m-%d_%H.%M.%S")-qh3server_sample.stacks -n 'profile-997 /execname == "qh3server"/ { @[ustack(100)] = count(); }'

# generate the flame graph
stackcollapse.pl 2023-12-07_23.14.47-qh3server_sample.stacks | flamegraph.pl > ./$(date +"%Y-%m-%d_%H.%M.%S")-qh3server-pretty-graph.svg
```
qh3server
[![qh3server flamegraph](https://github.com/arunmudhaliar/gearsdk/blob/wip/docs/fgraph/2023-12-07_23.27.45-qh3server-pretty-graph.svg "qh3server flamegraph")](https://github.com/arunmudhaliar/gearsdk/blob/wip/docs/fgraph/2023-12-07_23.27.45-qh3server-pretty-graph.svg "qh3server flamegraph")

*formatted by https://pandao.github.io/editor.md/en.html*