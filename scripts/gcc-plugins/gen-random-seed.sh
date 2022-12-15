#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

if [ ! -f "$1" ]; then
	SEED="NIXOS_RANDSTRUCT_SEED"
	echo "const char *randstruct_seed = \"$SEED\";" > "$1"
	HASH=`echo -n "$SEED" | sha256sum | cut -d" " -f1 | tr -d ' \n'`
	echo "#define RANDSTRUCT_HASHED_SEED \"$HASH\"" > "$2"
fi
