#!/usr/bin/env bash
set -euo pipefail

#───────────────────────────────────────────────────────────────────────────────
# infra/scripts/build.sh
# Builds the Linux PECL extension against local quiche/ sources
#───────────────────────────────────────────────────────────────────────────────
#
#  NEW IN THIS REVISION
#  --------------------
#  • clean_extension(): scrubs all intermediate objects created by libtool
#    (*.la *.lo *.o) **and** any .dep dependency files before we call make.
#  • CPPFLAGS now also injects  -I$(pwd)/include  (our own headers live there
#    since session.h, cancel.h, … were moved out of src/).
#  • Slightly nicer log message order; nothing functional changed.
#
#  Everything that was *already* uncommented in the original script is still
#  present and executed.

cd "$(dirname "${BASH_SOURCE[0]}")/../../"
source infra/scripts/utils.sh
cd extension

clean_extension() {
  message "🧹 Cleaning previous build artefacts…" 6 0
  (
    # If a Makefile exists from an earlier configure run, invoke make clean
    if [[ -f Makefile ]]; then make clean || true; fi
    # libtool drops .la and .lo files all over src/ — get rid of them
    find src -type f \( -name '*.lo' -o -name '*.la' -o -name '*.so' -o -name '*.o' -o -name '*.dep' \) -delete
    # .libs/ holds PIC objects; wipe it for good measure
    rm -rf src/.libs

  )
}

build_extension() {
  message "🔨 Entering extension/ directory…" 5 0
  message "📦 Setting up quiche include and lib paths…" 6 0
  export CPPFLAGS="-I$(pwd)/include -I$(pwd)/../quiche/include"
  export LDFLAGS="-L$(pwd)/../quiche/target/release \
                  -Wl,-rpath=$(pwd)/../quiche/target/release"
  clean_extension
  message "⚙️  Running phpize8.4…" 4 0
  phpize8.4
  message "🛠  Configuring for PHP 8.4…" 4 0
  ./configure --with-php-config=/usr/bin/php-config8.4
  message "🏗  Compiling extension…" 4 0
  make -j"$(nproc)"
  message "🚚 Installing extension…" 4 0
  sudo make install
  message "✅ Extension built and installed successfully!" 10 0
}

update_release_date() {
  local today
  today=$(date -u '+%Y-%m-%d')
  sed -i "s|<date>.*</date>|<date>${today}</date>|" package.xml
  echo "✔ Release date updated to ${today}"
}

run_act_publish() {
  bin/act -j publish -P ubuntu-latest=catthehacker/ubuntu:act-latest
}

main() {
  message "🚀 Starting PECL build…" 3 0
  build_extension
  message "🎉 Build complete. You can now run ‘make unit’ to verify." 2 0
  # update_release_date
  # run_act_publish
  cd ../../
}

main "$@"



# @TODO v1.0.0 we need to run "cargo build --release" inside the quiche directory once