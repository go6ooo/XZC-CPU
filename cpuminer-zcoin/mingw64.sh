./autogen.sh

CURL_PREFIX=/c/msys64/mingw64/lib
SSL_PREFIX=/c/msys64/mingw64/lib

#original
CFLAGS="-DCURL_STATICLIB -DOPENSSL_NO_ASM -DUSE_ASM -O3 -march=native -static -static-libgcc -static-libstdc++ -lcurl -lffi -lz"

CPPFLAGS=""

# icon
windres res/icon.rc icon.o 

./configure --build=x86_64-w64-mingw32 --with-crypto=$SSL_PREFIX --with-curl=$CURL_PREFIX CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="icon.o"

make

strip -p --strip-debug --strip-unneeded cpuminer.exe

if [ -e sign.sh ] ; then
. sign.sh
fi