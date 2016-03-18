function include_gtest()
   if os.get() == "windows" then
      home = string.gsub(os.getenv("USERPROFILE"), "\\", "/")
   else
      home = os.getenv("HOME")
   end
   local gtest_path = path.join(home, ".conan/data/gtest/1.7.0/RFerraro/stable/package")
   
   links { "gtest", "gtest_main" }
   
   filter {"action:vs2015", "configurations:Debug", "platforms:x64"}
      local gtest_hash = "f27dcd2d5e453cb01f9324a959461903ce8116a0"
      libdirs { path.join(gtest_path, gtest_hash, "lib") }
      includedirs { path.join(gtest_path, gtest_hash, "include") }
      
   filter {"action:vs2015", "configurations:Release", "platforms:x64"}
      local gtest_hash = "df81ad20137149d7a51276fd3e24009b45e5964a"
      libdirs { path.join(gtest_path, gtest_hash, "lib") }
      includedirs { path.join(gtest_path, gtest_hash, "include") }

   filter {"action:vs2015", "configurations:Debug", "platforms:x86"}
      local gtest_hash = "d9719aa212abfb01ad27d8207d37440982a68e66"
      libdirs { path.join(gtest_path, gtest_hash, "lib") }
      includedirs { path.join(gtest_path, gtest_hash, "include") }

   filter {"action:vs2015", "configurations:Release", "platforms:x86"}
      local gtest_hash = "79c71dbda360a634692376cd2ed5aafa18493952"
      libdirs { path.join(gtest_path, gtest_hash, "lib") }
      includedirs { path.join(gtest_path, gtest_hash, "include") }
end