#!/bin/sh
if [ $# -gt 0 ]; then
    FILE="$1"
    shift
    if [ -f "$FILE" ]; then
        INFO="$(head -n 1 "$FILE")"
    fi
else
    echo "Usage: $0 <filename>"
    exit 1
fi
if [ -e "$(which git)" ]; then
    # clean 'dirty' status of touched files that haven't been modified
    git diff >/dev/null 2>/dev/null
    
    # get commit hash
    GIT_COMMIT_ID="$(git rev-parse --short=9 HEAD 2>/dev/null)"
    
    # mark dirty if needed
    if [ -n "$GIT_COMMIT_ID" ]; then
        if ! git diff-index --quiet HEAD -- 2>/dev/null
            then $GIT_COMMIT_ID="${GIT_COMMIT_ID}-dirty" ; fi
    fi

    # get a string like "2012-04-10 16:27:19 +0200"
    TIME="$(git log -n 1 --format="%ci")"
fi

if [ -n "$GIT_COMMIT_ID" ]; then
    NEWINFO="#define GIT_COMMIT_ID \"$GIT_COMMIT_ID\""
else
    NEWINFO="// No build information available"
fi

# only update build.h if necessary
if [ "$INFO" != "$NEWINFO" ]; then
    echo "$NEWINFO" >"$FILE"
    echo "#define BUILD_DATE \"$TIME\"" >>"$FILE"
fi
