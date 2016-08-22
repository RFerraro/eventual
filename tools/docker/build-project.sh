#!/bin/bash

set -e

ToolsPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SourcePath="$( cd "$ToolsPath/../.." && pwd )"

#todo: probably should write something in *.yml for consistency...

ci_analyse_coverage=0
ci_post_coverage=0

if [[ "$CONFIGURATION" == "Debug" ]]; then
    ci_analyse_coverage=1
    
    if [[ "$CI" == "true" ]]; then
        ci_post_coverage=1
    fi
fi

function usage
{
    printf "Usage: %s [<options>] <build-path>\n" $(basename $0)
    echo ""
    echo "Available options:"
    echo "    -a     analyse code coverage [default: ${ci_analyse_coverage}]"
    echo "    -p     post code coverage (implies -a) [default: ${ci_post_coverage}]"
    echo ""
    echo "Environment variables used for building:"
    echo "    CC                path to the C compiler"
    echo "    CXX    	        path to the C++ compiler"
    echo "    CONFIGURATION     the build configuration"
    echo ""
    echo "Environment variables used for code coverage:"
    echo "    CI                defines default behavior for code coverage analysis."
}

echo $@

while getopts ":aph?" opt ; do
    case "$opt" in
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

echo "Configuring: cmake -DCMAKE_BUILD_TYPE=\"${CONFIGURATION}\" $SourcePath"
cmake -DCMAKE_BUILD_TYPE="${CONFIGURATION}" $SourcePath

echo "Building: make -j\"$(nproc)\""
make -j"$(nproc)"

#todo: make check?
echo "Running Tests..."
./library/test/bin/Test

if [ $ci_analyse_coverage -eq 1 ]; then 

	mkdir -p gcov
	pushd gcov
	
	source_path="${SourcePath}/library/eventual"
	gcno_files="../library/test/CMakeFiles/Test.dir/*.gcno"
	
	if [[ "${CC}" = "clang" ]]; then
		coverage_command="llvm-cov gcov -lp ${gcno_files}"
	else
	    coverage_command="gcov -lp ${gcno_files}"
	fi
	
	echo "Running: ${coverage_command} | grep -A 3 \"File '$source_path\""
	$coverage_command | grep -A 3 "File '${source_path}"
	
	if [ $ci_post_coverage -eq 1 ]; then
		include_pattern="*$( echo "${source_path}" | tr / \# )*.gcov"
		gcov_files=$(find . -type f -name "${include_pattern}" | while read data; do echo -f ${data}; done | xargs)
	
		bash <(curl -s https://codecov.io/bash) -X gcov -X coveragepy -X fix -Z -F "${CC}" ${gcov_files}
	fi
	
	popd

fi

popd
