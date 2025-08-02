FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV RUSTUP_HOME=/usr/local/rustup \
    CARGO_HOME=/usr/local/cargo \
    PATH=/usr/local/cargo/bin:$PATH

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      software-properties-common gnupg2 apt-transport-https lsb-release ca-certificates \
      build-essential autoconf pkg-config make re2c bison ninja-build clang \
      curl unzip git cmake ccache \
 && rm -rf /var/lib/apt/lists/*

RUN add-apt-repository ppa:ondrej/php -y \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      php8.1-cli php8.1-dev php8.1-common php8.1-zip php8.1-curl \
      php8.1-xml php8.1-mbstring \
 && rm -rf /var/lib/apt/lists/*

RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --profile minimal --default-toolchain stable \
 && rustup component add rustfmt \
 && rm -rf /root/.cargo/bin/rustup-init

RUN curl -sS https://getcomposer.org/installer | php -- --install-dir=/usr/local/bin --filename=composer

WORKDIR /workspace

COPY libcurl /workspace/libcurl
RUN cd /workspace/libcurl && \
    ./buildconf --force && \
    ./configure --disable-shared --with-ssl && \
    make -j$(nproc) && \
    make install

COPY ./extension /workspace/extension
COPY ./quiche /workspace/quiche
RUN cd extension && \
    phpize && \
    ./configure --with-quicpro-async --with-quiche=/workspace/quiche && \
    make -j$(nproc) && \
    make install

CMD ["tail", "-f", "/dev/null"]