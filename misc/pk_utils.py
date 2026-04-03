import os
import urllib.request
import tarfile

def pk_print(to_print):
    print("PopcornFX: " + to_print);

def download_pk_ifn(pk_version, pk_license, platform, target_folder):
    pk_package_name = f"PopcornFX-{pk_version}-{pk_license}-{platform}"

    if not os.path.exists(target_folder + pk_package_name):
        pk_server_url = "https://downloads.popcornfx.com/godot-packages/"
        pk_package_archive = pk_package_name + ".tar.xz"

        pk_print("Downloading " + pk_server_url + pk_package_archive)
        urllib.request.urlretrieve(pk_server_url + pk_package_archive, target_folder + pk_package_archive)

        pk_print("Extracting " + pk_package_archive)
        with tarfile.open(target_folder + pk_package_archive) as f:
            f.extractall(target_folder + pk_package_name)
        os.remove(target_folder + pk_package_archive)
    else:
        pk_print("Using " + pk_package_name)

    return pk_package_name