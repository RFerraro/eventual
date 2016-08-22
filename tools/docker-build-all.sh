#!/bin/bash

set -e

RepoPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
SourcePath=/usr/src/eventual/src
BuildPath=/usr/src/eventual/build
LocalConanData=/tmp/conan
ConanData=/tmp/conan
BuildScript="$SourcePath/tools/docker/build-project.sh"

function test-image
{
    docker_tag=$1
    shift
    configuration=$1

    echo "running $docker_tag with configuration $configuration"
    
    #todo: migrate to data volumes...
    docker run \
        -it \
        --rm \
        --privileged \
        -v $RepoPath:$SourcePath:ro \
        -v $LocalConanData:$ConanData \
        -e CONFIGURATION="${configuration}" \
        $docker_tag bash -ci "$BuildScript $BuildPath"
}

mkdir -p $LocalConanData

test-image rferraro/cxx-travis-ci:gcc-5.3.0 Debug
test-image rferraro/cxx-travis-ci:clang-3.8.0 Debug
test-image rferraro/cxx-travis-ci:clang-3.7.1 Debug

test-image rferraro/cxx-travis-ci:gcc-5.3.0 Release
test-image rferraro/cxx-travis-ci:clang-3.8.0 Release
test-image rferraro/cxx-travis-ci:clang-3.7.1 Release

docker images
