#/bin/bash
mkdir -p samples/small
mkdir -p samples/big
# small files
for i in {0..1000};
do
	dd if=/dev/urandom of="./samples/small/L$i" bs=1K count=5K
done
dd if=/dev/urandom of=./samples/small/A bs=1K count=10K
# duplicates
cp ./samples/small/A ./samples/small/B
cp ./samples/small/A ./samples/small/C
# file which size is not a multiple of 512 bits / 64 bytes
dd if=/dev/urandom of=./samples/small/D bs=1K count=10K
echo 'I need padding' >> ./samples/small/D
# empty files
touch ./samples/small/EMPTY
touch ./samples/small/EMPTY2
touch ./samples/small/EMPTY3
# big files
for i in {0..19};
do
	dd if=/dev/urandom of="./samples/big/H$i" bs=1K count=100K
done
# 1 duplicate
cp ./samples/big/H1 ./samples/big/DUP
