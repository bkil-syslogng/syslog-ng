#!/bin/sh

get_gradle() {
  export GRADLE_HOME=$SLNG_CACHE/gradle-2.9
  export PATH=$GRADLE_HOME/bin:$PATH

  if ! which gradle; then
    wget https://downloads.gradle.org/distributions/gradle-2.9-bin.zip || exit 1
    unzip gradle-2.9-bin.zip -d "$SLNG_CACHE" || exit 1
    rm -v gradle-2.9-bin.zip
  fi
  true
}

get_gradle "$@"
