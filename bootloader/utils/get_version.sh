#!/bin/bash

sam_ba_version=
if [[ -f sam_ba_monitor.h ]]; then
	sam_ba_version="$(sed -n '/SAM_BA_VERSION/s/.*"\(.*\)".*/\1/p' sam_ba_monitor.h)"
fi
[[ -n $sam_ba_version ]] || sam_ba_version=1.1

rev_count="$(git rev-list HEAD | wc -l)"
sha="$(git rev-parse --short HEAD)"

dirty=
[[ -n "$(git status --porcelain --untracked=no 2>/dev/null)" ]] && dirty="+"

echo "v${sam_ba_version}-${rev_count}-g${sha}${dirty}"
