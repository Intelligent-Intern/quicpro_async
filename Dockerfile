FROM php:8.3-cli

RUN apt-get update && apt-get install -y git zip unzip curl \
    && pecl channel-update pecl.php.net

RUN pecl channel-discover intelligentinternpecl.z1.web.core.windows.net

CMD ["pecl", "install", "quicpro/quicpro_async"]
