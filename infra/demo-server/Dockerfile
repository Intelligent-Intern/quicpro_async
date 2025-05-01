# infra/php84/Dockerfile
FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y \
      ca-certificates curl gnupg2 lsb-release apt-transport-https \
 && curl -fsSL https://packages.sury.org/php/apt.gpg \
      | gpg --dearmor -o /usr/share/keyrings/sury-php.gpg \
 && printf 'deb [signed-by=/usr/share/keyrings/sury-php.gpg] https://packages.sury.org/php/ %s main\n' "$(lsb_release -sc)" \
      | tee /etc/apt/sources.list.d/sury-php.list \
 && apt-get update \
 && apt-get install -y \
      php8.4-cli php8.4-dev php8.4-xml php8.4-common \
      php8.4-opcache php8.4-curl php8.4-mbstring \
      git build-essential autoconf pkg-config \
      re2c bison libssl-dev zlib1g-dev ccache \
      rustc cargo \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN phpize8.4 \
 && ./configure --with-php-config=/usr/bin/php-config8.4 --enable-quicpro-sched \
 && make -j"$(nproc)" \
 && make install \
 && echo "extension=quicpro_async.so" > /etc/php/8.4/cli/conf.d/20-quicpro_async.ini

ENTRYPOINT ["php8.4", "-d", "variables_order=EGPCS"]
