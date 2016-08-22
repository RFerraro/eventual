#!/bin/bash

set -e

ToolsPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SourcePath="$( cd "$ToolsPath/../.." && pwd )"

if [[ "${CC}" = "clang" ]]; then 
    coverageTool="$SourcePath/tools/travis/llvm-gcov.sh"
else
    coverageTool="gcov"
fi

ci_branch=master #$TRAVIS_BRANCH
ci_repo_slug=github/gitignore #$TRAVIS_REPO_SLUG
ci_build_id=0 #$TRAVIS_BUILD_ID
ci_job_id=0 #$TRAVIS_JOB_NUMBER
ci_commit_sha=deadbee #$TRAVIS_COMMIT
ci_post_coverage=0

function usage
{
    printf "Usage: %s [<options>] <build-path>\n" $(basename $0)
    echo ""
    echo "Available options:"
    echo "    -b     branch (TRAVIS_BRANCH) [default: ${ci_branch}]"
    echo "    -s     repo slug (TRAVIS_REPO_SLUG) [default: ${ci_repo_slug}]"
    echo "    -i     build id (TRAVIS_BUILD_ID) [default: ${ci_build_id}]"
    echo "    -j     job id (TRAVIS_JOB_NUMBER) [default: ${ci_job_id}]"
    echo "    -c     commit sha (TRAVIS_COMMIT) [default: ${ci_commit_sha}]"
    echo "    -p     post coverage results [default: ${ci_post_coverage}]"
    echo ""
    echo "Environment variables:"
    echo "    CC     path to the C compiler"
    echo "    CXX    path to the C++ compiler"
}

while getopts ":b:s:i:j:c:ph?" opt ; do
    case "$opt" in
        b)
            ci_branch=$OPTARG
            ;;
        s)
            ci_repo_slug=$OPTARG
            ;;
        i)
            ci_build_id=$OPTARG
            ;;
        j)
            ci_job_id=$OPTARG
            ;;
        c)
            ci_commit_sha=$OPTARG
            ;;
        p)
            ci_post_coverage=1
            ;;
        h|\?)
        	echo "help"
            usage
            exit 0
            ;;
    esac
done

shift $(expr $OPTIND - 1)
BuildPath=`echo $1 | sed 's#/$##'`

if [ "$BuildPath" == "" ]; then
    usage
    exit 1
fi

pushd $SourcePath

$coverageTool --version
#ls -la /tmp/build/test/bin

coveralls -r $SourcePath \
          -i include/eventual \
          -i include/eventual/detail \
          --dump $BuildPath/coverage.json \
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
captureDay=$(( 10#$(date -u -d@$captureTimestamp +%d) ))
captureDoubleMinutes=$(( ($captureTimestamp - $captureMidnight) / 120 ))

# "10#" forces bash to treat the result as a decimal value (rather then oct...)
months=$(((($captureYear - $epochYear) * 12) + ($(( 10#$captureMonth )) - $(( 10#$epochMonth )))))

service_number=$(printf "%d%02d%03d" $months $captureDay $captureDoubleMinutes)
runTime=$(date -u -d@$captureTimestamp -Iseconds)

#not supported until jq 1.5...
updates="{ 
\"service_number\" : \"$service_number\", 
\"service_branch\" : \"$ci_branch\", 
\"service_build_url\" : \"https://travis-ci.org/$ci_repo_slug/builds/$ci_build_id\",
\"service_job_id\" : \"$ci_job_id\",
\"commit_sha\" : \"$ci_commit_sha\",
\"run_at\" : \"$runTime\"
}"

jq --version

echo $updates | jq '.'
echo "origin: https://github.com/${ci_repo_slug}.git"

cat $BuildPath/coverage.json | 
jq  \
   --arg service_number "$service_number" \
   --arg service_branch "$ci_branch" \
   --arg service_build_url "https://travis-ci.org/$ci_repo_slug/builds/$ci_build_id" \
   --arg service_job_id "$ci_job_id" \
   --arg commit_sha "$ci_commit_sha" \
   --arg run_at "$runTime" \
   --arg origin "https://github.com/${ci_repo_slug}.git" \
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
   ' > $BuildPath/coverage-update.json

if [ $ci_post_coverage -eq 0 ]; then
	echo "dumping raw output..."   
	echo "<omitted>"
	#cat $BuildPath/coverage-update.json
	exit 0
fi

#curl -sSi -F json_file=@$BuildPath/coverage-update.json https://coveralls.io/api/v1/jobs
status_code=$(curl -s -o /dev/stderr -w "%{http_code}" -F json_file=@$BuildPath/coverage-update.json https://coveralls.io/api/v1/jobs)
echo ""

if [ $status_code -ne 200 ]; then
	echo "Error posting to coveralls, Status code: ${status_code}"
	echo "Content:"
	cat $BuildPath/coverage-update.json
	exit 1
fi

popd
    
# [foreach .[] as $item ([]; if $item == null then null elif $item < 1 then 0 else 1 end; .)]
# foreach  .source_files[].coverage[] as $item ([]; if $item == null then null elif $item < 1 then 0 else 1 end; .)
# .source_files[].coverage[] |= if . == null then null elif . < 1 then 0 else 1 end
