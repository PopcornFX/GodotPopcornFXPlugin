#!/usr/bin/env python
import os
import shutil

from misc.pk_utils import download_pk_ifn
from misc.pk_vsproj import generate_vs_project
from misc.pk_platform_methods import create_macos_universal_libs, PlatformPaths
from misc.pk_platform_methods import combine_ios_libs, create_ios_xcframework

POPCORNFX_VERSION = "2.24.2"
POPCORNFX_LICENSE = "Godot"

def link_popcornfx(bin_dir):

    # -------------------------------------------------
    # POPCORNFX SDK
    # -------------------------------------------------

    if env["popcornfx_dev"]:
        pk_root_dir = "../../"
    else:
        thirdparty_path = "thirdparty/"
        os.makedirs(thirdparty_path, exist_ok=True)
        pk_package_name = download_pk_ifn(POPCORNFX_VERSION, POPCORNFX_LICENSE, env['platform'], thirdparty_path);
        pk_root_dir = thirdparty_path + pk_package_name + "/PopcornFX/"

    # -------------------------------------------------
    # POPCORNFX LIBRARIES
    # -------------------------------------------------

    pk_libs = []

    if env.editor_build:
        pk_libs.extend([
            "PK-AssetBakerLib",
            "PK-Plugin_CodecImage_TIFF",
        ])

    pk_libs.extend([
        "PK-Plugin_CodecImage_PKIM",
        "PK-Plugin_CodecImage_DDS",
        "PK-Plugin_CodecImage_PKM",
        "PK-Plugin_CodecImage_PNG",
        "PK-Plugin_CodecImage_PVR",
        "PK-Plugin_CodecImage_TGA",
        #"PK-Plugin_CodecImage_TIFF", # Link issue, temp disable. See src/integration/internal/popcornfx_startup.cpp
        "PK-Plugin_CodecImage_JPG",
        "PK-Plugin_CodecImage_HDR",
        "PK-Plugin_CodecImage_EXR",
        "PK-ZLib",
        "PK-RenderHelpers",
        "PK-Plugin_CompilerBackend_CPU_VM",
        "PK-ParticlesToolbox",
        "PK-Runtime",
    ])

    pk_freetype_version = "freetype-2.13.3"

    pk_platform_libs = []

    # -------------------------------------------------
    # CONFIGURATION
    # -------------------------------------------------

    if env["target"] == "template_debug" or env["target"] == "editor":
        if env["dev_build"] == True:
            pk_conf_short = "_d"
            env.Append(CPPDEFINES=["PK_DEBUG"])
        else:
            pk_conf_short = "_r"
            env.Append(CPPDEFINES=["PK_RELEASE"])
    elif env["target"] == "template_release":
        pk_conf_short = "_s"
        env.Append(CPPDEFINES=["PK_RETAIL"])

    # -------------------------------------------------
    # PLATFORM: WINDOWS
    # -------------------------------------------------

    if env['platform'] == "windows":
        pk_platform = "vs2022_x64"
        pk_lib_prefix = ""
        pk_lib_ext = ".lib"

        pk_platform_libs.extend([
            "Version.lib",
            "Psapi.lib",
        ])
        if env["target"] != "template_release":
            pk_platform_libs.extend([
                "User32.lib", # For asserts
            ])

        if env["dev_build"] and env["target"] == "template_debug":
            pk_platform_freetype = "vs2022_x64/Debug"
        else:
            pk_platform_freetype = "vs2022_x64_mt/Release"
        # Ignore missing PDBs warnings because of freetype
        env.Append(LINKFLAGS=["/ignore:4099"])

    # -------------------------------------------------
    # PLATFORM: LINUX
    # -------------------------------------------------

    elif env['platform'] == "linux":
        pk_platform = "gmake_linux_x64"
        pk_lib_prefix = "lib"
        pk_lib_ext = ".a"

        if env["dev_build"] == True:
            pk_platform_libs.extend(["bfd"])

        pk_platform_freetype = "linux64"

    # -------------------------------------------------
    # PLATFORM: MACOS
    # -------------------------------------------------

    elif env['platform'] == "macos":
        pk_lib_prefix = "lib"
        pk_lib_ext = ".a"

        if env.editor_build:
            pk_platform_libs.extend(["z"]) # For Freetype

        macos_platform_paths = {
            "arm64":     PlatformPaths("gmake_macosx_ARM64",        "macosx_ARM64"),
            "x86_64":    PlatformPaths("gmake_macosx_x64",          "macosx_x64"),
            "universal": PlatformPaths("gmake_macosx_universal",    "macosx_universal"),
        }

        macos_universal_libs = []
        if env['arch'] == "universal":
            macos_universal_libs = create_macos_universal_libs(
                env,
                POPCORNFX_LICENSE,
                pk_root_dir,
                pk_libs,
                pk_lib_prefix,
                pk_lib_ext,
                pk_conf_short,
                pk_freetype_version,
                macos_platform_paths
            )

        pk_platform, pk_platform_freetype = macos_platform_paths[env['arch']]

    # -------------------------------------------------
    # PLATFORM: IOS
    # -------------------------------------------------

    elif env['platform'] == "ios":
        pk_platform = "gmake_ios64"
        pk_lib_prefix = "lib"
        pk_lib_ext = ".a"

        dev_suffix = ".dev" if env.dev_build else ""
        ios_merged_libs = combine_ios_libs(
            env, POPCORNFX_LICENSE,
            pk_root_dir, pk_libs, pk_lib_prefix, pk_lib_ext,
            pk_conf_short, pk_platform,
            "{}libpopcornfx-runtime.{}.{}{}.a".format(bin_dir, env["platform"], env["target"], dev_suffix)
        )

    # -------------------------------------------------
    # PLATFORM: ANDROID
    # -------------------------------------------------

    elif env['platform'] == "android":
        pk_platform = "gmake_android64"
        pk_lib_prefix = "lib"
        pk_lib_ext = ".a"

        pk_platform_libs.extend(["log"])

    # -------------------------------------------------
    # INCLUDE PATHS
    # -------------------------------------------------

    env.Append(CPPPATH=[
        pk_root_dir + "Runtime/",
        pk_root_dir + "Runtime/include",
        pk_root_dir + "Runtime/include/license/" + POPCORNFX_LICENSE,
    ])
    # Add AssetBaker include path
    if env.editor_build:
        env.Append(CPPPATH=[
            pk_root_dir + "Tools/",
        ])

    # -------------------------------------------------
    # LINK LIBRARIES
    # -------------------------------------------------

    # Add libraries path
    env.Append(LIBPATH=[pk_root_dir + "Runtime/bin/{}/{}/".format(POPCORNFX_LICENSE, pk_platform)])

    # Add libraries
    pk_libs_full = []
    for lib in pk_libs:
        pk_libs_full.append(pk_lib_prefix + lib + pk_conf_short + pk_lib_ext)
    env.Append(LIBS=pk_libs_full)

    if env.editor_build:
        # Needed by asset baker lib
        env.Append(LIBPATH=[pk_root_dir + "Runtime/libs/{}/lib/{}/".format(pk_freetype_version, pk_platform_freetype)])
        env.Append(LIBS=[
            pk_lib_prefix + "freetype" + pk_lib_ext,
        ])

    env.Append(LIBS=pk_platform_libs)

    # -------------------------------------------------
    # DEPENDENCIES
    # -------------------------------------------------

    if env['platform'] == "macos" and env['arch'] == "universal":
        return macos_universal_libs
    
    if env['platform'] == "ios":
        return ios_merged_libs

    return []

#----------------------------------------------------------------------------

_custom_opts = [
    BoolVariable("popcornfx_dev", "Use PopcornFX dev libraries", False),
    BoolVariable("vsproj", "Generate a Visual Studio solution", False),
    ("godot_path", "Path to the Godot repository", ""),
]

_custom_keys = [opt[0] for opt in _custom_opts]
_saved_args = {k: ARGUMENTS.pop(k) for k in _custom_keys if k in ARGUMENTS}

env = SConscript("godot-cpp/SConstruct")

ARGUMENTS.update(_saved_args)
opts = Variables([], ARGUMENTS)
for opt in _custom_opts:
    opts.Add(*opt if isinstance(opt, tuple) else opt)
opts.Update(env)
Help(opts.GenerateHelpText(env))

output_dir = "addons/popcornfx/"
libs_dir = output_dir + "libs/"
bin_dir = output_dir + "bin/"

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

#----------------------------------------------------------------------------

src_dir = "src/"

env.Append(CPPPATH=[src_dir])

sources = Glob(src_dir + "*.cpp")
for root, subdirs, files in os.walk(src_dir):
    for subdir in subdirs:
        # Skip editor folders if not an editor build
        if subdir == "editor" and not env.editor_build:
            continue
        # Add sources
        sources = sources + Glob(root + "/" + subdir + "/*.cpp")

#----------------------------------------------------------------------------

# Store all obj files in a dedicated folder
build_dir = "build_{}/".format(env["platform"])
os.makedirs(build_dir, exist_ok=True)
env["SHOBJPREFIX"] = "#{}obj/".format(build_dir)

if env.get("is_msvc", False):
    env.Append(CCFLAGS=['/Fd' + build_dir])

#----------------------------------------------------------------------------

if env["build_library"]:

    pk_depends = link_popcornfx(bin_dir)

    dev_suffix = ".dev" if env.dev_build else ""

    if env["platform"] == "macos":
        library = env.SharedLibrary(
            "{}libpopcornfx.{}.{}{}.framework/libpopcornfx.{}.{}{}".format(
                libs_dir,
                env["platform"], env["target"], dev_suffix,
                env["platform"], env["target"], dev_suffix
            ),
            source=sources,
        )

    elif env["platform"] == "ios":
        if env["ios_simulator"]:
            #library = env.StaticLibrary(
            #    "{}libpopcornfx.{}.{}.simulator.a".format(bin_dir, env["platform"], env["target"]),
            #    source=sources,
            #)
            print("Error: iOS simulator not supported.")
            Exit(255)
        else:
            library = env.StaticLibrary(
                "{}libpopcornfx.{}.{}{}.a".format(bin_dir, env["platform"], env["target"], dev_suffix),
                source=sources,
            )

    else:
        library = env.SharedLibrary(
            "{}libpopcornfx{}{}".format(libs_dir, env["suffix"], env["SHLIBSUFFIX"]),
            source=sources,
        )
    
    if pk_depends:
        Depends(library, pk_depends)

    if env["platform"] == "ios":
        xcframework_runtime = create_ios_xcframework(env, bin_dir, libs_dir, "libpopcornfx-runtime", env["target"], dev_suffix)
        Depends(xcframework_runtime, library)
        xcframework_godotcpp = create_ios_xcframework(env, "godot-cpp/bin/", libs_dir, "libgodot-cpp", env["target"], dev_suffix, ".arm64")
        Depends(xcframework_godotcpp, xcframework_runtime)
        xcframework_plugin = create_ios_xcframework(env, bin_dir, libs_dir, "libpopcornfx", env["target"], dev_suffix)
        Depends(xcframework_plugin, xcframework_godotcpp)
        final_target = xcframework_plugin
    else:
        final_target = library

    Default(final_target)

#----------------------------------------------------------------------------

demo_dir = "demo/"
with_demo = os.path.isdir(demo_dir)

def copy_to_demo(target, source, env):
    demo_addon_dir = demo_dir + "addons/popcornfx/"
    os.makedirs(demo_addon_dir, exist_ok=True)
    print("Copying to " + demo_addon_dir)
    demo_libs_dir = demo_addon_dir + "/libs/"
    shutil.rmtree(demo_libs_dir, ignore_errors=True)
    shutil.copytree(libs_dir, demo_libs_dir, dirs_exist_ok=True)
    shutil.copy2(output_dir + "/popcornfx.gdextension", demo_addon_dir)
    shutil.copytree(output_dir + "assets/", demo_addon_dir + "assets/", dirs_exist_ok=True)
    demo_icons_dir = demo_addon_dir + "/icons/"
    if not os.path.isdir(demo_icons_dir):
        shutil.copytree(output_dir + "icons/", demo_icons_dir)
    

if with_demo and env["build_library"]:
    copy_command = Command("copy_to_demo", [], copy_to_demo)
    Depends(copy_command, final_target)
    Default(copy_command)

#----------------------------------------------------------------------------

if env["vsproj"]:
    if os.name != "nt":
        print("Error: The `vsproj` option is only usable on Windows with Visual Studio.")
        Exit(255)

    # Add msvs tool to have access to MSVSProject and MSVSSolution
    msvs_tool = Tool("msvs")
    msvs_tool.generate(env)

    env.vs_incs = []
    env.vs_srcs = []

    vcxproj_user_file = build_dir + "godot-popcornfx.vcxproj.user"
    if with_demo and not os.path.isfile(vcxproj_user_file):
        demo_path = os.path.abspath("demo")
        shutil.copy2("misc/godot-popcornfx.patch.vcxprojuser", vcxproj_user_file)
        # Read in the file
        with open(vcxproj_user_file, 'r') as file:
            filedata = file.read()
        # Replace the target string
        filedata = filedata.replace('DEMO_PATH', demo_path)
        # Write the file out again
        with open(vcxproj_user_file, 'w') as file:
            file.write(filedata)

    generate_vs_project(sources, env, GetOption("num_jobs"), build_dir)

#----------------------------------------------------------------------------