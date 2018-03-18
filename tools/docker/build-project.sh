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

function write_conan_settings
{
# based on default settings from conan 0.26.1
cat > ~/.conan/settings.yml << EOF

os:
    Windows:
    Linux:
    Macos:
    Android:
        api_level: ANY
    iOS:
        version: ["7.0", "7.1", "8.0", "8.1", "8.2", "8.3", "9.0", "9.1", "9.2", "9.3", "10.0", "10.1", "10.2", "10.3"]
    FreeBSD:
    SunOS:
    Arduino:
        board: ANY
arch: [x86, x86_64, ppc64le, ppc64, armv6, armv7, armv7hf, armv8, sparc, sparcv9, mips, mips64, avr]
compiler:
    sun-cc:
        version: ["5.10", "5.11", "5.12", "5.13", "5.14"]
        threads: [None, posix]
        libcxx: [libCstd, libstdcxx, libstlport, libstdc++]
    gcc:
        version: ["4.1", "4.4", "4.5", "4.6", "4.7", "4.8", "4.9", "5.1", "5.2", "5.3", "5.4", "6.1", "6.2", "6.3", "6.4", "7.1", "7.2"]
        libcxx: [libstdc++, libstdc++11]
        threads: [None, posix, win32] #  Windows MinGW
        exception: [None, dwarf2, sjlj, seh] # Windows MinGW
    Visual Studio:
        runtime: [MD, MT, MTd, MDd]
        version: ["8", "9", "10", "11", "12", "14", "15"]
    clang:
        version: ["3.3", "3.4", "3.5", "3.6", "3.7", "3.8", "3.9", "4.0", "5.0"]
        libcxx: [libstdc++, libstdc++11, libc++]
    apple-clang:
        version: ["5.0", "5.1", "6.0", "6.1", "7.0", "7.3", "8.0", "8.1", "9.0"]
        libcxx: [libstdc++, libc++]
build_type: [None, Debug, Release]

EOF
}

function install_conan
{
    echo "Installing latest Conan C/C++ package manager."
    pip install conan
    conan --version
    
    #echo "Configuring custom settings for Conan."
    #write_conan_settings

    # Update conan data directory for caching by the CI environment.
    echo "Setting conan data path to: /tmp/conan"
    conan config set storage.path=/tmp/conan
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

install_conan

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
