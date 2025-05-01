# infra/php84/Dockerfile
# ─────────────────────────────────────────────────────────────────────────────
# Base on the official PHP 8.4 CLI image to avoid third-party PPAs entirely.
# This image already includes phpize, php-config, and dev headers.
#───────────────────────────────────────────────────────────────────────────────
FROM php:8.4-cli

ARG DEBIAN_FRONTEND=noninteractive

# 1) Install build dependencies
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      git \
      build-essential \
      autoconf \
      pkg-config \
      re2c \
      bison \
      libssl-dev \
      zlib1g-dev \
      ccache \
      rustc \
      cargo \
 && rm -rf /var/lib/apt/lists/*

# 2) Build & install the quicpro_async extension
WORKDIR /app
COPY . /app

RUN phpize \
 && ./configure --enable-quicpro-sched \
 && make -j"$(nproc)" \
 && make install \
 && echo "extension=quicpro_async.so" > /usr/local/etc/php/conf.d/20-quicpro_async.ini

# 3) Default entrypoint runs PHP CLI with our variables order
ENTRYPOINT ["php", "-d", "variables_order=EGPCS"]
