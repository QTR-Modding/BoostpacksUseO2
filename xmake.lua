set_xmakever("3.0.8")

local project_root = os.scriptdir()

if is_plat("windows") then
    add_cxflags(
        "/Brepro",
        "/experimental:deterministic",
        '/d1trimfile:"' .. project_root .. '"',
        '/pathmap:"' .. project_root .. '"=.',
        {
            tools = "cl",
            force = true
        })
    add_shflags("/Brepro", "/PDBALTPATH:%_PDB%", {
        force = true
    })
end

local sfsemcp = os.getenv("SFSEMCP_PATH") or "lib/sfse-mcp"
local staging_dir = path.join(project_root, "build", "staging")
option("sf1sdk_dir")
    set_default("../SF1SDK")
    set_showmenu(true)
    set_description("Directory containing the SF1SDK checkout")
option_end()

includes(get_config("sf1sdk_dir") or "../SF1SDK")
add_requires("spdlog v1.16.0", { configs = { header_only = false, wchar = true, std_format = true } })

local plugin_name = "O2BoostRecharge"
local plugin_version = "1.0.1"
local plugin_author = "Quantumyilmaz"

set_project(plugin_name)
set_version(plugin_version)
set_license("GPL-3.0-or-later")
set_languages("c++23")
set_warnings("allextra")

local plugin_description = "Makes boostpack fuel recharge consume O2 while preserving native boost behavior"
local major, minor, patch = plugin_version:match("^(%d+)%.(%d+)%.(%d+)$")
local packed_version = tonumber(major) * 16777216 + tonumber(minor) * 65536 + tonumber(patch) * 16

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

target(plugin_name)
    add_defines("_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING", "NOMINMAX", "WIN32_LEAN_AND_MEAN")
    set_kind("shared")
    add_rules("sf1sdk.plugin", { stage = true, symbols = false })
    add_deps("SF1SDK")
    add_packages("spdlog")
    add_files("src/Version.rc")
    add_syslinks("user32")
    add_defines(
        'PLUGIN_NAME="' .. plugin_name .. '"',
        'PLUGIN_AUTHOR="' .. plugin_author .. '"',
        'PLUGIN_DESCRIPTION="' .. plugin_description .. '"',
        'PLUGIN_VERSION_TEXT="' .. plugin_version .. '"',
        "PLUGIN_VERSION_MAJOR=" .. major,
        "PLUGIN_VERSION_MINOR=" .. minor,
        "PLUGIN_VERSION_PATCH=" .. patch,
        "PLUGIN_VERSION_PACKED=" .. packed_version
    )

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src", path.join(sfsemcp, "include"), path.join(sfsemcp, "lib/clib-utils-qtr/include"))
    set_pcxxheader("src/PCH.h")
    add_installfiles("config/O2BoostRecharge.ini", { prefixdir = "SFSE/Plugins" })
    add_installfiles("COPYING", "EXCEPTIONS", "THIRD_PARTY_NOTICES.md")
    add_installfiles("LICENSES/*.txt", { prefixdir = "LICENSES" })

    set_installdir(staging_dir)

target("RechargeMathTests")
    set_kind("binary")
    set_default(false)
    add_files("tests/recharge_math_tests.cpp")
    add_headerfiles("src/RechargeMath.h")
    add_includedirs("src")

target("SettingsTests")
    set_kind("binary")
    set_default(false)
    add_deps("SF1SDK")
    add_packages("spdlog")
    add_files("tests/settings_tests.cpp", "src/Settings.cpp")
    add_headerfiles("src/ConfigParsing.h", "src/Settings.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")
