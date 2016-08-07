#!/bin/bash

set -e
ToolsPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SourcePath="$( cd "$ToolsPath/../.." && pwd )"

#todo: probably should write something in *.yml for consistency...

ci_branch=master #$TRAVIS_BRANCH
ci_repo_slug=github/gitignore #$TRAVIS_REPO_SLUG
ci_build_id=0 #$TRAVIS_BUILD_ID
ci_job_id=0 #$TRAVIS_JOB_NUMBER
ci_commit_sha=deadbee #$TRAVIS_COMMIT
ci_build_configuration=Debug
ci_analyse_coverage=0
ci_post_coverage=0

function usage
{
    printf "Usage: %s [<options>] <build-path>\n" $(basename $0)
    echo ""
    echo "Available options:"
    echo "    -B     branch (TRAVIS_BRANCH) [default: ${ci_branch}]"
    echo "    -b     build id (TRAVIS_BUILD_ID) [default: ${ci_build_id}]"
    echo "    -S     commit sha (TRAVIS_COMMIT) [default: ${ci_commit_sha}]"
    echo "    -s     repo slug (TRAVIS_REPO_SLUG) [default: ${ci_repo_slug}]"
    echo "    -j     job id (TRAVIS_JOB_NUMBER) [default: ${ci_job_id}]"
    echo "    -c     build configuration type (Debug, Release) [default: ${ci_build_configuration}]"
    echo "    -a     analyse code coverage [default: ${ci_analyse_coverage}]"
    echo "    -p     post code coverage (implies -a) [default: ${ci_post_coverage}]"
    echo ""
    echo "Environment variables:"
    echo "    CC     path to the C compiler"
    echo "    CXX    path to the C++ compiler"
}

echo $@

while getopts ":B:b:S:s:j:c:aph?" opt ; do
    case "$opt" in
        B)
            ci_branch=$OPTARG
            ;;
        b)
            ci_build_id=$OPTARG
            ;;
        S)
            ci_commit_sha=$OPTARG
            ;;
        s)
            ci_repo_slug=$OPTARG
            ;;
        j)
            ci_job_id=$OPTARG
            ;;
        c)
            ci_build_configuration=$OPTARG
            ;;
        a)
            ci_analyse_coverage=1
            ;;
        p)
            ci_analyse_coverage=1
            ci_post_coverage=1
            ;;
        h|\?)
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

mkdir -p $BuildPath
pushd $BuildPath
conan install $SourcePath -s build_type=$ci_build_configuration --build=missing

echo "Starting project Build..."
cmake -DCMAKE_BUILD_TYPE="${ci_build_configuration}" $SourcePath
make -j"$(nproc)"
#cmake --build .

#todo: make check?
echo "Running Tests..."
./test/bin/Test

if [ $ci_analyse_coverage -eq 1 ]; then 
	
	if [ $ci_post_coverage -eq 1 ]; then 
		bash $ToolsPath/post_coverage_results.sh -b $ci_branch -s $ci_repo_slug -i $ci_build_id -j $ci_job_id -c $ci_commit_sha -p $BuildPath
	else
		bash $ToolsPath/post_coverage_results.sh -b $ci_branch -s $ci_repo_slug -i $ci_build_id -j $ci_job_id -c $ci_commit_sha $BuildPath
	fi
	
fi

popd

