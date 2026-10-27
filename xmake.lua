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

local commonlibsf = os.getenv("COMMONLIBSF_PATH") or "lib/commonlibsf"
local sfsemcp = os.getenv("SFSEMCP_PATH") or "lib/sfse-mcp"
local staging_dir = path.join(project_root, "build", "staging")
includes(commonlibsf)

set_project("O2BoostRecharge")
set_version("1.0.1")
set_license("GPL-3.0-or-later")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

target("O2BoostRecharge")
    add_defines("_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING")
    add_rules("commonlibsf.plugin", {
        name = "O2BoostRecharge",
        author = "Quantumyilmaz",
        description = "Makes boostpack fuel recharge consume O2 while preserving native boost behavior",
        options = {
            address_library = true,
            no_struct_use = false,
            layout_dependent = true
        }
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src", path.join(sfsemcp, "include"), path.join(sfsemcp, "lib/clib-utils-qtr/include"))
    set_pcxxheader("src/PCH.h")
    add_installfiles("config/O2BoostRecharge.ini", { prefixdir = "SFSE/Plugins" })
    add_installfiles("COPYING", "EXCEPTIONS", "THIRD_PARTY_NOTICES.md")
    add_installfiles("LICENSES/*.txt", { prefixdir = "LICENSES" })

    on_config(function(target)
        target:set("installdir", staging_dir)
        target:remove("installfiles", target:symbolfile())
    end)

    before_build(function(target)
        assert(path.absolute(target:installdir()) == path.absolute(staging_dir),
            "build output must remain in the project staging directory")
    end)

target("RechargeMathTests")
    set_kind("binary")
    set_default(false)
    add_files("tests/recharge_math_tests.cpp")
    add_headerfiles("src/RechargeMath.h")
    add_includedirs("src")

target("SettingsTests")
    set_kind("binary")
    set_default(false)
    add_deps("commonlibsf")
    add_files("tests/settings_tests.cpp", "src/Settings.cpp")
    add_headerfiles("src/ConfigParsing.h", "src/Settings.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")
