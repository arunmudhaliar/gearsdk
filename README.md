# gearsdk

ubuntu build
 - sudo apt install clang
 - sudo apt-get install libc++-dev
 - sudo apt-get install g++multilib

 docker jenkins
 - sudo usermod -a -G docker jenkins
 - docker build -t qserver-exp .
 - docker run -it qserver-exp