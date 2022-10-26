# Find duplicated files
Code created during recording the [C++ filesystem API and optimizing SHA-256](https://youtu.be/hc1nne_APW8) video

C++ filesystem API, SHA-256, threading

# Building
## Stage 1
```
g++ -std=c++17 -Wall -Wextra -O2 -march=native -mtune=native -I./ -o dedup ./dedup.cpp ./sha256.cpp
```

## Stage 2
```
g++ -std=c++17 -Wall -Wextra -O2 -march=native -mtune=native -I./ -o dedup2 ./dedup2.cpp ./flo-shani.c
```

## Stage 3
```
g++ -std=c++17 -Wall -Wextra -O2 -march=native -mtune=native -I./ -o dedup3 ./dedup3.cpp ./flo-shani.c -lpthread
```

# Links
[FIPS 180-4](https://csrc.nist.gov/publications/detail/fips/180/4/final)

[Intel SHA Extensions](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sha-extensions.html)

[FLO-SHANI-AESNI](https://github.com/armfazh/flo-shani-aesni)

