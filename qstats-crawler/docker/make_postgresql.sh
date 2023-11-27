cd ./postgresql-15.5
mkdir ./build
./configure --prefix=/app/postgresql-15.5/build
make clean
make release
make install
cd ..
rm -r ./qstats-crawler/libs/linux
mkdir ./qstats-crawler/libs/linux
cp -a /app/postgresql-15.5/build/lib/. ./qstats-crawler/libs/linux
ls ./qstats-crawler/libs/linux/