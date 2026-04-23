import os
from collections.abc import Mapping
from typing import Iterator
from pathlib import Path

# Unmodified from godot/method.py
def find_visual_c_batch_file(env):
    from SCons.Tool.MSCommon.vc import get_default_version, get_host_target, find_batch_file, find_vc_pdir

    # Syntax changed in SCons 4.4.0.
    from SCons import __version__ as scons_raw_version

    scons_ver = env._get_major_minor_revision(scons_raw_version)

    msvc_version = get_default_version(env)

    if scons_ver >= (4, 4, 0):
        (host_platform, target_platform, _) = get_host_target(env, msvc_version)
    else:
        (host_platform, target_platform, _) = get_host_target(env)

    if scons_ver < (4, 6, 0):
        return find_batch_file(env, msvc_version, host_platform, target_platform)[0]

    # Scons 4.6.0+ removed passing env, so we need to get the product_dir ourselves first,
    # then pass that as the last param instead of env as the first param as before.
    # We should investigate if we can avoid relying on SCons internals here.
    product_dir = find_vc_pdir(msvc_version, env=env)
    return find_batch_file(msvc_version, host_platform, target_platform, product_dir)[0]

# Unmodified from godot/method.py
def glob_recursive(pattern, node="."):
    from SCons import Node
    from SCons.Script import Glob

    results = []
    for f in Glob(str(node) + "/*", source=True):
        if type(f) is Node.FS.Dir:
            results += glob_recursive(pattern, f)
    results += Glob(str(node) + "/" + pattern, source=True)
    return results

# /!\ Modified ! (from godot/method.py)
def add_to_vs_project(env, sources, build_dir):
    for x in sources:
        #if type(x) == type(""):
        #    fname = env.File(x).path
        #else:
        #    fname = env.File(x)[0].path
        fname = x.path
        pieces = fname.split(".")
        if len(pieces) > 0:
            basename = pieces[0]
            basename = basename.replace("\\\\", "/")
            rel_basename = os.path.relpath(basename, build_dir)
            if os.path.isfile(basename + ".h"):
                env.vs_incs.append(rel_basename + ".h")
            elif os.path.isfile(basename + ".hpp"):
                env.vs_incs.append(rel_basename + ".hpp")
            if os.path.isfile(basename + ".c"):
                env.vs_srcs.append(rel_basename + ".c")
            elif os.path.isfile(basename + ".cpp"):
                env.vs_srcs.append(rel_basename + ".cpp")


# /!\ Modified ! (from godot/method.py)
def generate_vs_project(sources, env, num_jobs, build_dir, project_name="godot-popcornfx"):
    batch_file = find_visual_c_batch_file(env)

    if batch_file:

        class ModuleConfigs(Mapping):
            # This version information (Win32, x64, Debug, Release) seems to be
            # required for Visual Studio to understand that it needs to generate an NMAKE
            # project. Do not modify without knowing what you are doing.
            PLATFORMS = ["x64"]
            PLATFORM_IDS = ["x86_64"]
            CONFIGURATIONS = ["editor", "template_release", "template_debug"]
            DEV_SUFFIX = ".dev" if env["dev_build"] else ""
    
            @staticmethod
            def for_every_variant(value):
                return [value for _ in range(len(ModuleConfigs.CONFIGURATIONS) * len(ModuleConfigs.PLATFORMS))]
    
            def __init__(self):
                shared_targets_array = []
                self.names = []
                self.arg_dict = {
                    "variant": [],
                    "runfile": shared_targets_array,
                    "buildtarget": shared_targets_array,
                    "cpppaths": [],
                    "cppdefines": [],
                    "cmdargs": [],
                }
                self.add_mode()  # default
    
            def add_mode(
                self,
                name: str = "",
                includes: str = "",
                cli_args: str = "",
                defines=None,
            ):
                if defines is None:
                    defines = []
                self.names.append(name)
                variants = [
                    f'{config}{f"_[{name}]" if name else ""}|{platform}'
                    for config in ModuleConfigs.CONFIGURATIONS
                    for platform in ModuleConfigs.PLATFORMS
                ]
                self.arg_dict["variant"] += variants
                if env["godot_path"]:
                    self.arg_dict["runfile"] += [
                        f'{Path(env["godot_path"]).resolve()}\\bin\\godot.windows.{config}{ModuleConfigs.DEV_SUFFIX}{".double" if env["precision"] == "double" else ""}.{plat_id}{f".{name}" if name else ""}.exe'
                        for config in ModuleConfigs.CONFIGURATIONS
                        for plat_id in ModuleConfigs.PLATFORM_IDS
                    ]
                else:
                    self.arg_dict["runfile"] += [""] * len(variants)
                self.arg_dict["cpppaths"] += ModuleConfigs.for_every_variant(env["CPPPATH"] + [includes])
                self.arg_dict["cppdefines"] += ModuleConfigs.for_every_variant(list(env["CPPDEFINES"]) + defines)
                self.arg_dict["cmdargs"] += ModuleConfigs.for_every_variant(cli_args)
    
            def build_commandline(self, commands):
                configuration_getter = (
                    "$(Configuration"
                    + "".join([f'.Replace("{name}", "")' for name in self.names[1:]])
                    + '.Replace("_[]", "")'
                    + ")"
                )
    
                common_build_prefix = [
                    'cmd /V /C set "plat=$(PlatformTarget)"',
                    '(if "$(PlatformTarget)"=="x64" (set "plat=x86_amd64"))',
                    'call "' + batch_file + '" !plat!',
                ]
    
                # Windows allows us to have spaces in paths, so we need
                # to double quote off the directory. However, the path ends
                # in a backslash, so we need to remove this, lest it escape the
                # last double quote off, confusing MSBuild
                common_build_postfix = [
                    "--directory=\"$(ProjectDir.TrimEnd('\\'))\\..\"",
                    "platform=windows",
                    f"target={configuration_getter}",
                #    "progress=no",
                    "-j%s" % num_jobs,
                ]
                if env["api_version"]:
                    common_build_postfix.append("api_version=" + env["api_version"])

                if env["debug_symbols"]:
                    common_build_postfix.append("debug_symbols=yes")

                if env["dev_build"]:
                    common_build_postfix.append("dev_build=yes")

                if env["popcornfx_dev"]:
                    common_build_postfix.append("popcornfx_dev=yes")

                #if env["dev_mode"]:
                #    common_build_postfix.append("dev_mode=yes")
    
                #elif env["tests"]:
                #    common_build_postfix.append("tests=yes")
    
                #if env["custom_modules"]:
                #    common_build_postfix.append("custom_modules=%s" % env["custom_modules"])
    
                if env["precision"] == "double":
                    common_build_postfix.append("precision=double")
    
                result = " ^& ".join(common_build_prefix + [" ".join([commands] + common_build_postfix)])
                return result
    
            # Mappings interface definitions
    
            def __iter__(self) -> Iterator[str]:
                for x in self.arg_dict:
                    yield x
    
            def __len__(self) -> int:
                return len(self.names)
    
            def __getitem__(self, k: str):
                return self.arg_dict[k]
    
        add_to_vs_project(env, sources, build_dir)
    
        for header in glob_recursive("**/*.h"):
            env.vs_incs.append(str(header))

        for header in glob_recursive("**/*.hpp"):
            env.vs_incs.append(str(header))
    
        module_configs = ModuleConfigs()
    
        if env.get("module_mono_enabled"):
            mono_defines = [("GD_MONO_HOT_RELOAD",)] if env.editor_build else []
            module_configs.add_mode(
                "mono",
                cli_args="module_mono_enabled=yes",
                defines=mono_defines,
            )
    
        env["MSVSBUILDCOM"] = module_configs.build_commandline("scons")
        if env["godot_path"]:
            env["MSVSREBUILDCOM"] = module_configs.build_commandline(f'scons vsproj=yes godot_path="{Path(env["godot_path"])}"')
        env["MSVSCLEANCOM"] = module_configs.build_commandline("scons --clean")
        if not env.get("MSVS"):
            env["MSVS"]["PROJECTSUFFIX"] = ".vcxproj"
            env["MSVS"]["SOLUTIONSUFFIX"] = ".sln"
        proj = env.MSVSProject(
            target=["#" + build_dir + project_name + env["MSVSPROJECTSUFFIX"]],
            incs=env.vs_incs,
            srcs=env.vs_srcs,
            misc=['.clang-format'],
            auto_build_solution=0,
            **module_configs,
        )
        env.Default(proj)

        projects = [build_dir + project_name + env["MSVS"]["PROJECTSUFFIX"]]

        if env["godot_path"]:
            vsproj = str(Path(env["godot_path"]) / "godot.vcxproj")
            if os.path.isfile(vsproj):
                projects.append(vsproj)
            else:
                print("WARNING: godot_path supplied but " + vsproj + " doesn't exists. Skipping.'")
        
        solution = env.MSVSSolution(
            target = "#" + build_dir + project_name + env['MSVSSOLUTIONSUFFIX'],
            projects = projects,
            variant = module_configs.arg_dict["variant"]
        )
        env.Default(solution)

    else:
        print("Could not locate Visual Studio batch file to set up the build environment. Not generating VS project.")