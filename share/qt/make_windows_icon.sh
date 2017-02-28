#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/pinkcoin.ico

convert ../../src/qt/res/icons/pinkcoin-16.png ../../src/qt/res/icons/pinkcoin-32.png ../../src/qt/res/icons/pinkcoin-48.png ${ICON_DST}
