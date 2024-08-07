--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

set_toolchains "gcc"
set_languages "c++20"

set_defaultmode "debug"
set_policy("build.warning", true)

add_rules("mode.release", "mode.debug")
add_requires("sfml", "spdlog", "doctest", "glm")

if is_mode "debug" then
  local sanitize = { "-fsanitize=undefined", "-fsanitize=leak" }
  local warnings = { "-Wall", "-Wextra", "-Weffc++", "-Wshadow" }
  -- "-Wfatal-errors"

  add_cxxflags(warnings, sanitize, "-pedantic-errors")
  add_ldflags(sanitize, "-lubsan")
end

target "program"
set_default(true)
set_kind "binary"
add_packages("sfml", "spdlog", "glm")
add_files "src/**.cpp"
add_includedirs "inc"

target "test"
set_default(false) -- does not run, unless called explictly (xmake run test)
set_kind "binary"
add_packages("doctest", "sfml", "glm")
add_files("test/doctest.cpp", "test/src/**.cpp")
add_includedirs "inc"
add_deps "program"                           -- ensure program is already compiled
add_defines "DOCTEST_CONFIG_USE_STD_HEADERS" -- global #define

--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
