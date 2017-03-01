fusermount -u $(pwd)/mountdir
rm sfs
touch sfs
../src/sfs $(pwd)/sfs $(pwd)/mountdir
