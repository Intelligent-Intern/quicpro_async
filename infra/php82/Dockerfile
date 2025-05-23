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
      php8.2-cli php8.2-dev php8.2-common php8.2-zip php8.2-curl \
      php8.2-xml php8.2-mbstring \
 && rm -rf /var/lib/apt/lists/*

RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --profile minimal --default-toolchain stable \
 && rustup component add rustfmt \
 && rm -rf /root/.cargo/bin/rustup-init

RUN curl -sS https://getcomposer.org/installer | php -- --install-dir=/usr/local/bin --filename=composer

WORKDIR /workspace

CMD ["tail", "-f", "/dev/null"]
