--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

set_toolchains "gcc"
set_languages "c++20"

set_defaultmode "debug"
set_policy("build.warning", true)

add_rules("mode.release", "mode.debug")
add_requires "sfml"

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
add_packages "sfml"
add_files "src/**.cpp"
add_includedirs "inc"

--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
