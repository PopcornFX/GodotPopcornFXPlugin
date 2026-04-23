# Godot PopcornFX Plugin

> /!\ **Alpha version** — Expect bugs and missing or broken features. Use at your own risk and feel free to report any issues you encounter.

Integrates the **PopcornFX Runtime SDK** into **Godot** as a GDExtension.
* **Version:** `2.24.2`
* **Godot:** `4.5` and `4.6`
* **Godot-cpp:** `10.0.0-rc1`
* **Supported platforms:** `Windows`, `MacOS`, `Linux`, `iOS`, `Android`

## Setup

### Installing from downloaded archive

Simply download `addon.zip` found in the [releases](https://github.com/PopcornFX/GodotPopcornFXPlugin/releases) tab, extract it, and copy the `addons/` folder into your project's root directory.

### Installing from GitHub source repository

Clone this repository and compile using scons for the needed platform and target.
```
scons platform=windows target=editor
```
Then use the `addons/popcornfx` folder in your project.

## Quick Links: Documentation and Support

* [**Plugin** documentation](https://documentation.popcornfx.com/PopcornFX/latest/plugins/godot-plugin/)
* [PopcornFX **Editor** documentation](https://documentation.popcornfx.com/)
* [PopcornFX **Discord** server](https://discord.gg/4ka27cVrsf)

## License

See [LICENSE.md](/LICENSE.md).