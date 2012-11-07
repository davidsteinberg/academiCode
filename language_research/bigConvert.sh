#!/bin/bash

# REQUIRES ffmpeg to work
# I recommend getting it, cause it's awesome

# This code accepts a directory as a parameter
# makes a wav folder in the directory
# and converts all mp3s in the directory to wavs (in the wav folder)

cd $1

mkdir -p "wav"

find "./" -name '*.MP3' -exec sh -c 'mv -f "$0" "${0%%.MP3}.mp3"' {} \;

find "./" -name '*.mp3' -exec sh -c 'ffmpeg -i "$0" -n "wav/${0%%.mp3}.wav"' {} \;

cd ~

exit