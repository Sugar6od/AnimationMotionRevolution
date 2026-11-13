-- include subprojects
includes("lib/commonlibsse")

-- enable commonlib-shared support
set_config("commonlib_xbyak", true)

-- set project constants
set_project("AnimationMotionRevolution")
set_version("1.6.2")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.release", "mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- define targets
target("AnimationMotionRevolution")
    add_rules("commonlibsse.plugin", {
        name = "AnimationMotionRevolution",
        author = "Alejandro and SugarGod",
        description = "Animation Motion Revolution for Skyrim 1.7.104"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")