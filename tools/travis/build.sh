#!/bin/bash

set -e

docker_image="${DOCKER_NAME}:${DOCKER_TAG}"

#host path
conan_data_cache="/tmp/conan"

#docker image paths
docker_build_path="/usr/src/eventual/build"
docker_source_path="/usr/src/eventual"
docker_conan_data="/tmp/conan"
docker_build_script="${docker_source_path}/tools/docker/build-project.sh"

#captures ci environment variables
ci_environment=`bash <(curl -s https://codecov.io/env)`

echo "invoking ${docker_image} with command: ${docker_build_script} ${docker_build_path}"
    
#todo: migrate to data volumes...
docker run \
    -it \
    --rm \
    --privileged \
    -v "${TRAVIS_BUILD_DIR}":$docker_source_path \
    -v $conan_data_cache:$docker_conan_data \
    -e CONFIGURATION \
    $ci_environment \
    $docker_image bash -ci "${docker_build_script} ${docker_build_path}"

