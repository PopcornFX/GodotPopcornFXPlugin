from SCons.Script import Builder, Action
import shutil
from collections import namedtuple

PlatformPaths = namedtuple("PlatformPaths", ["pk_platform", "pk_platform_freetype"])

# -------------------------------------------------
# BUILDER: lipo (macOS universal libs)
# -------------------------------------------------

def lipo_action(target, source, env):
    out_path = target[0].srcnode().abspath
    lipo = "$APPLE_TOOLCHAIN_PATH/usr/bin/lipo"
    env.Execute(
        lipo + ' -create -output "' + out_path + '" ' +
        " ".join([('"' + lib.srcnode().abspath + '"') for lib in source])
    )

#----------------------------------------------------------------------------

def create_macos_universal_libs(env, pk_license,
                                pk_root_dir, pk_libs, pk_lib_prefix, pk_lib_ext,
                                pk_conf_short, pk_freetype_version, macos_platform_paths):
    lipo_builder = Builder(action=Action(lipo_action, 'Creating universal lib $TARGET'))
    env.Append(BUILDERS={"LipoLib": lipo_builder})

    base_pk = pk_root_dir + "Runtime/bin/{}/".format(pk_license)
    base_ft = pk_root_dir + "Runtime/libs/{}/lib/".format(pk_freetype_version)

    universal_libs = []

    for lib in pk_libs:
        lib_name = pk_lib_prefix + lib + pk_conf_short + pk_lib_ext
        output   = base_pk + macos_platform_paths["universal"].pk_platform + "/" + lib_name
        src_x64  = base_pk + macos_platform_paths["x86_64"].pk_platform    + "/" + lib_name
        src_arm  = base_pk + macos_platform_paths["arm64"].pk_platform     + "/" + lib_name

        result = env.LipoLib(output, [src_x64, src_arm])
        universal_libs.append(result)

    if env.editor_build:
        ft_name  = pk_lib_prefix + "freetype" + pk_lib_ext
        output   = base_ft + macos_platform_paths["universal"].pk_platform_freetype + "/" + ft_name
        src_x64  = base_ft + macos_platform_paths["x86_64"].pk_platform_freetype    + "/" + ft_name
        src_arm  = base_ft + macos_platform_paths["arm64"].pk_platform_freetype     + "/" + ft_name

        result = env.LipoLib(output, [src_x64, src_arm])
        universal_libs.append(result)

    return universal_libs

# -------------------------------------------------
# BUILDER: libtool (iOS static lib merge)
# -------------------------------------------------

def libtool_action(target, source, env):
    lib_path = target[0].srcnode().abspath
    libtool = "$APPLE_TOOLCHAIN_PATH/usr/bin/libtool"
    env.Execute(
        libtool + ' -static -o "' + lib_path + '" ' + " ".join([('"' + lib.srcnode().abspath + '"') for lib in source])
    )

#----------------------------------------------------------------------------

def combine_ios_libs(env, POPCORNFX_LICENSE,
                     pk_root_dir, pk_libs, pk_lib_prefix, pk_lib_ext,
                     pk_conf_short, pk_platform, output_path):
    libtool_builder = Builder(action=Action(libtool_action, 'Creating merged lib $TARGET'))
    env.Append(BUILDERS={"LibtoolLib": libtool_builder})

    base_pk = pk_root_dir + "Runtime/bin/{}/{}/".format(POPCORNFX_LICENSE, pk_platform)

    source_libs = []
    for lib in pk_libs:
        lib_name = pk_lib_prefix + lib + pk_conf_short + pk_lib_ext
        source_libs.append(base_pk + lib_name)

    result = env.LibtoolLib(output_path, source_libs)
    return result

# -------------------------------------------------
# BUILDER: xcodebuild (iOS xcframework)
# -------------------------------------------------

def xcodebuild_xcframework_action(target, source, env):
    out_path = target[0].srcnode().abspath
    shutil.rmtree(out_path, ignore_errors=True)
    cmd = '$APPLE_TOOLCHAIN_PATH/usr/bin/xcodebuild -create-xcframework'
    for lib in source:
        cmd += ' -library "{}"'.format(lib.srcnode().abspath)
    cmd += ' -output "{}"'.format(out_path)
    env.Execute(cmd)

#----------------------------------------------------------------------------

def create_ios_xcframework(env, bin_dir, libs_dir, lib_name, target_name, dev_suffix, arch=""):
    lib = "{}{}.ios.{}{}{}.a".format(bin_dir, lib_name, target_name, dev_suffix, arch)
    output = "{}{}.ios.{}{}.xcframework".format(libs_dir, lib_name, target_name, dev_suffix)

    result = env.Command(output, lib, Action(xcodebuild_xcframework_action, 'Creating xcframework $TARGET'))
    env.NoCache(result)
    env.AlwaysBuild(result)
    return result
