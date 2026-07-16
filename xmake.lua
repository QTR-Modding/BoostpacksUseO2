set_xmakever("3.0.8")

if is_plat("windows") then
    local project_root = os.projectdir()
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
includes(commonlibsf)

set_project("O2BoostRecharge")
set_version("0.4.1")
set_license("GPL-3.0-or-later")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

target("O2BoostRecharge")
    add_defines("_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING")
    add_rules("commonlibsf.plugin", {
        name = "O2BoostRecharge",
        author = "Quant",
        description = "Makes boostpack fuel recharge consume O2 while preserving native boost behavior",
        options = {
            address_library = true,
            no_struct_use = false,
            layout_dependent = true
        }
    })

    add_files("src/main.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    add_installfiles("config/O2BoostRecharge.ini", { prefixdir = "SFSE/Plugins" })

target("RechargeMathTests")
    set_kind("binary")
    add_files("tests/recharge_math_tests.cpp")
    add_headerfiles("src/RechargeMath.h")
    add_includedirs("src")
