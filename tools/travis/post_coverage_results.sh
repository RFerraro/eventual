#!/bin/bash

#TRAVIS_BUILD_DIR="~/Code/eventual"

if [[ "${COMPILER}" = "clang" ]]; then 
    coverageTool="$TRAVIS_BUILD_DIR/tools/travis/llvm-gcov.sh"
else
    coverageTool="gcov-5"
fi

coveralls -r $TRAVIS_BUILD_DIR \
          -i include/eventual \
          -i include/eventual/detail \
          --dump $TRAVIS_BUILD_DIR/coverage.json \
          --gcov-options '\-lp' \
          --gcov $coverageTool

# encode a coveralls build number as: MMDDmmm
# XX  : number of months after 12/31/2015
# DD  : Day of current month (utc)
# mmm : minutes after midnight utc / 2

#12/31/2015 00:00 Z
epochYear=2015
epochMonth=12

#git commit timestamp
captureTimestamp=$(git log -1 --format="%ct")
captureDate=$(date -u -d@$captureTimestamp +%F)
captureMidnight=$(date -u -d "$captureDate 0" +%s)

captureYear=$(date -u -d@$captureTimestamp +%Y)
captureMonth=$(date -u -d@$captureTimestamp +%m)
captureDay=$(date -u -d@$captureTimestamp +%d)
captureDoubleMinutes=$((($captureTimestamp - $captureMidnight) / 120))

months=$(((($captureYear - $epochYear) * 12) + ($captureMonth - $epochMonth)))

service_number=$(printf "%d%02d%03d" $months $captureDay $captureDoubleMinutes)
runTime=$(date -u -d@$captureTimestamp --rfc-3339=seconds)

#not supported until jq 1.5...
updates="{ 
\"service_number\" : \"$service_number\", 
\"service_branch\" : \"$TRAVIS_BRANCH\", 
\"service_build_url\" : \"https://travis-ci.org/$TRAVIS_REPO_SLUG/builds/$TRAVIS_BUILD_ID\",
\"service_job_id\" : \"$TRAVIS_JOB_NUMBER\",
\"commit_sha\" : \"$TRAVIS_COMMIT\",
\"run_at\" : \"$runTime\"
}"

jq --version

echo $updates | jq '.'

cat $TRAVIS_BUILD_DIR/coverage.json | 
jq  \
   --arg service_number "$service_number" \
   --arg service_branch "$TRAVIS_BRANCH" \
   --arg service_build_url "https://travis-ci.org/$TRAVIS_REPO_SLUG/builds/$TRAVIS_BUILD_ID" \
   --arg service_job_id "$TRAVIS_JOB_NUMBER" \
   --arg commit_sha "$TRAVIS_COMMIT" \
   --arg run_at "$runTime" \
   --arg origin "https://github.com/$TRAVIS_REPO_SLUG.git" \
   '
   .service_name = "custom" |
   .service_number = $service_number |
   .service_branch = $service_branch |
   .service_build_url = $service_build_url |
   .service_job_id = $service_job_id |
   .commit_sha = $commit_sha |
   .run_at = $run_at |
   .git.remotes[0] |= { "name" : "origin", "url" : $origin } |
   .source_files[].coverage[] |= if . == null then null elif . < 1 then 0 else 1 end
   ' > $TRAVIS_BUILD_DIR/coverage-update.json
    
# [foreach .[] as $item ([]; if $item == null then null elif $item < 1 then 0 else 1 end; .)]
# foreach  .source_files[].coverage[] as $item ([]; if $item == null then null elif $item < 1 then 0 else 1 end; .)
# .source_files[].coverage[] |= if . == null then null elif . < 1 then 0 else 1 end

curl -S -F json_file=@$TRAVIS_BUILD_DIR/coverage-update.json https://coveralls.io/api/v1/jobs
