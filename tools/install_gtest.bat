@ECHO OFF
REM Install Conan Packages for all build configurations/platforms.



cd ..

conan install . -s arch=x86 -s build_type=Release -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MT -s os=Windows -build=missing
move /Y conanbuildinfo.cmake conanbuildinfo_release_x86.cmake
conan install . -s arch=x86 -s build_type=Debug -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MTd -s os=Windows -build=missing
move /Y conanbuildinfo.cmake conanbuildinfo_debug_x86.cmake

conan install . -s arch=x86_64 -s build_type=Release -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MT -s os=Windows -build=missing
move conanbuildinfo.cmake conanbuildinfo_release_x86_64.cmake
conan install . -s arch=x86_64 -s build_type=Debug -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MTd -s os=Windows -build=missing
move conanbuildinfo.cmake conanbuildinfo_debug_x86_64.cmake

cd %~dp0
