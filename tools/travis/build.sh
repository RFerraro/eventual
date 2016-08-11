#!/bin/bash

set -e

#CONFIGURATION
#DOCKER_NAME
#DOCKER_TAG
#COVERALLS_REPO_TOKEN
#TRAVIS_BUILD_DIR
#TRAVIS_BRANCH
#TRAVIS_REPO_SLUG
#TRAVIS_BUILD_ID
#TRAVIS_JOB_NUMBER
#TRAVIS_COMMIT

docker_image="${DOCKER_NAME}:${DOCKER_TAG}"

#host path
conan_data_cache="/tmp/conan"

#docker image paths
docker_build_path="/usr/src/eventual/build"
docker_source_path="/usr/src/eventual"
docker_conan_data="/tmp/conan"
docker_build_script="${docker_source_path}/tools/travis/build-in-docker.sh"

script_options="\
-c ${CONFIGURATION} \
-B ${TRAVIS_BRANCH} \
-b ${TRAVIS_BUILD_ID} \
-S ${TRAVIS_COMMIT} \
-s ${TRAVIS_REPO_SLUG} \
-j ${TRAVIS_JOB_NUMBER} \
${docker_build_path}"

if [[ "$CONFIGURATION" == "Debug" ]]; then
    script_options="-p ${script_options}"
fi

echo "invoking ${docker_image} with command: ${docker_build_script} ${script_options}"
    
    #todo: migrate to data volumes...
    docker run \
        -it \
        --rm \
        --privileged \
        -v "${TRAVIS_BUILD_DIR}":$docker_source_path \
        -v $conan_data_cache:$docker_conan_data \
        -e COVERALLS_REPO_TOKEN="${COVERALLS_REPO_TOKEN}" \
        $docker_image bash -ci "${docker_build_script} ${script_options}"

