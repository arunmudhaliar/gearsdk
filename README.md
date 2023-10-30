# gearsdk

ubuntu build
 - sudo apt install clang
 - sudo apt-get install libc++-dev
 - sudo apt-get install g++multilib

 docker jenkins
 - sudo usermod -a -G docker jenkins
 - docker build -t qserver-exp .
 - docker run -it qserver-exp


- response header added
    Alternate-Protocol: quic:<QUIC server port>
    {
        .name = (uint8_t *) "Alternate-Protocol",
        .name_len = sizeof("Alternate-Protocol") - 1,

        .value = (uint8_t *) "quic:6121",
        .value_len = sizeof("quic:6121") - 1,
    },

- SPKI
 cat cert.crt |
      openssl x509 -inform pem -noout -outform pem -pubkey |
      openssl pkey -pubin -inform pem -outform der |
      openssl dgst -sha256 -binary |
      openssl enc -base64
[MCFtYhgL/+T4kkcV64TQTTAw0Q5Gq2360530xEr9lFs=]

- Chrome Canary
/Applications/Google\ Chrome\ Canary.app/Contents/MacOS/Google\ Chrome\ Canary ----enable-quic --origin-to-for-quic-on=localhost:6121 https://localhost:6121/whoami --ignore-certificate-errors-spki-list=<SPKI>