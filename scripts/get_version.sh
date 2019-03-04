#!/bin/bash

version_name=
if [[ $1 == --sam-ba ]]; then
    sam_ba_version=
    if [[ -f sam_ba_monitor.h ]]; then
        sam_ba_version="$(sed -n '/SAM_BA_VERSION/s/.*"\(.*\)".*/\1/p' sam_ba_monitor.h)"
    fi
    [[ -n $sam_ba_version ]] || sam_ba_version=1.1
    version_name="v$sam_ba_version"
elif [[ $1 == --branch ]]; then
    branch=$(git symbolic-ref HEAD 2>/dev/null | sed -n 's:^refs/heads/::p')
    if [[ -n $branch ]]; then
        version_name="$branch"
    fi
fi
[[ -n $version_name ]] && version_name="${version_name}-"

rev_count="$(git rev-list HEAD | wc -l)"
sha="$(git rev-parse --short HEAD)"

dirty=
[[ -n "$(git status --porcelain --untracked=no 2>/dev/null)" ]] && dirty="+"

echo -n "${version_name}${rev_count}-g${sha}${dirty}"
[[ -t 1 ]] && echo ""
