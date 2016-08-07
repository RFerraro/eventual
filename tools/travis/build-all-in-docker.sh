#!/bin/bash

set -e

RepoPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"
SourcePath=/usr/src/eventual/src
BuildPath=/usr/src/eventual/build
LocalConanData=/tmp/conan
ConanData=/tmp/conan
BuildScript="$SourcePath/tools/travis/build-in-docker.sh"

function test-image
{
    docker_tag=$1
    shift

    echo "running $docker_tag with args $@"
    
    #todo: migrate to data volumes...
    docker run \
        -it \
        --rm \
        --privileged \
        -v $RepoPath:$SourcePath:ro \
        -v $LocalConanData:$ConanData \
        $docker_tag bash -ci "$BuildScript $* $BuildPath"
}

mkdir -p $LocalConanData

test-image rferraro/cxx-travis-ci:gcc-5.3.0 -ac Debug
test-image rferraro/cxx-travis-ci:clang-3.8.0 -ac Debug
test-image rferraro/cxx-travis-ci:clang-3.7.1 -ac Debug

test-image rferraro/cxx-travis-ci:gcc-5.3.0 -c Release
test-image rferraro/cxx-travis-ci:clang-3.8.0 -c Release
test-image rferraro/cxx-travis-ci:clang-3.7.1 -c Release

docker images
