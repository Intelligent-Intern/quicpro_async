How to build **quicpro_async** from source on Linux, macOS and Windows.  

---

## 1 · Prerequisites

### Linux (Ubuntu / Debian)
~~~bash
sudo apt update
sudo apt install -y \
     php8.4-dev php-pear build-essential pkg-config \
     git curl cargo clang cmake ninja-build libssl-dev
~~~

### macOS (Homebrew) -- UNTESTED
~~~bash
brew install php@8.4 llvm cmake ninja rust openssl@3
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/openssl@3/lib/pkgconfig"
~~~

### Windows (MSVC) -- UNTESTED
* PHP x64 TS SDK
* Visual Studio 2022 + C++ toolset
* Rust (`rustup target add x86_64-pc-windows-msvc`)
* `vcpkg install openssl:x64-windows-static-md ninja`

---

## 2 · Clone & init submodule

~~~bash
git clone --recurse-submodules https://github.com/intelligent-intern/quicpro_async.git
cd quicpro_async
~~~

If you forgot `--recurse-submodules`:

~~~bash
git submodule update --init --recursive
~~~

---

## 3 · Build the extension

### Standard (system OpenSSL)

~~~bash
phpize
./configure --enable-quicpro_async
make -j$(nproc)
sudo make install
~~~

### Build quiche statically with its bundled BoringSSL

~~~bash
export QUICHE_BSSL_PATH="$(pwd)/quiche/deps/boringssl"
export OPENSSL_STATIC=yes
phpize
./configure --enable-quicpro_async
make -j$(nproc)
sudo make install
~~~

---

## 4 · Enable in PHP

CLI:
~~~ini
; /etc/php/8.4/cli/conf.d/30-quicpro_async.ini
extension=quicpro_async.so
~~~

FPM:
~~~ini
; /etc/php/8.4/fpm/conf.d/30-quicpro_async.ini
extension=quicpro_async.so
~~~
`systemctl restart php8.4-fpm`

Verify:

~~~bash
php -m | grep quicpro_async
~~~

---

## 5 · Docker (alpine)

~~~dockerfile
FROM php:8.4-cli-alpine

RUN apk add --no-cache g++ make perl bash cargo openssl-dev cmake ninja \
 && git clone --depth=1 --recurse-submodules https://github.com/intelligent-intern/quicpro_async.git /src \
 && cd /src \
 && phpize && ./configure --enable-quicpro_async \
 && make -j$(nproc) && make install

RUN echo "extension=quicpro_async.so" > /usr/local/etc/php/conf.d/quicpro_async.ini
CMD ["php", "-v"]
~~~

Build:

~~~bash
docker build -t php-quicpro .
~~~

---

## 6 · Running tests

~~~bash
make test          # C test‑suite
php tests/benchmark/bench.php 1000 100
~~~

---

## 7 · Updating quiche - TESTED on v0.22.0

~~~bash
cd quiche
git checkout v0.24.0
cd ..
git add quiche
git commit -m "Bump quiche to v0.24.0"
~~~

Rebuild the extension afterwards.

---

Happy compiling!  For issues see **CONTRIBUTING.md** or mail  
<jschultz@php.net>.