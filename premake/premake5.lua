dofile "include_gtest.lua"

workspace "Eventual"
   configurations { "Debug", "Release" }
   platforms { "x64", "x86" }
   location ("../build/" .. _ACTION)
   
   language "C++"
   targetdir "../bin/%{cfg.platform}/%{cfg.buildcfg}"
   includedirs { "../include" }
   flags { "StaticRuntime", "FatalWarnings", "Symbols" }
   editandcontinue "Off"
      
   pchheader "stdafx.h"
   
   --filter "action:vs2015"
   --   system "Windows"
      
   --filter "action:gmake"
   --   system "linux"
   
   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
            
   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "Off"
      
   filter "platforms:x64"
      architecture "x64"
      
   filter "platforms:x86"
      architecture "x86"
      
project "Samples"
   kind "ConsoleApp"
   targetname "samples"
   
   files { "../src/sample/*.cpp", "../src/sample/*.h" }
   vpaths { ["Header Files/*"] = "../src/sample/*.h", ["Source Files/*"] = "../src/sample/*.cpp" }
   
   filter "action:vs*"
      pchsource "../src/sample/stdafx.cpp"

project "Test"
   kind "ConsoleApp"
   targetname "test"
      
   files { "../test/*.cpp", "../test/*.h" }
   vpaths { ["Header Files/*"] = "../test/*.h", ["Source Files/*"] = "../test/*.cpp" }
   
   include_gtest()
   
   filter "action:vs*"
      pchsource "../test/stdafx.cpp"
   
   filter {"action:vs2015", "platforms:x64"}
      buildoptions { "/bigobj" } -- workaround for C1128
   