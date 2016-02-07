@ECHO OFF
REM Install Conan Packages for all build configurations/platforms.

conan install . -s arch=x86_64 -s build_type=Release -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MT -s os=Windows 
conan install . -s arch=x86_64 -s build_type=Debug -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MTd -s os=Windows
conan install . -s arch=x86 -s build_type=Release -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MT -s os=Windows
conan install . -s arch=x86 -s build_type=Debug -s compiler="Visual Studio" -s compiler.version=14 -s compiler.runtime=MTd -s os=Windows
