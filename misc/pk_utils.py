import os
import shutil
import urllib.request
import tarfile
import hashlib

from misc.pk_packages import PK_PACKAGES

PK_DOWNLOAD_URL = "https://auth.popcornfx.com/ws/downloadPlugin?id="
PK_FALLBACK_URL = "https://downloads.popcornfx.com/godot-packages/"

def _pk_print(to_print):
    print("PopcornFX: " + to_print);

def _sha256(filepath):
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def _download_validate_extract(url, download_target, extract_dir, expected_hash):
    try:
        urllib.request.urlretrieve(url, download_target)
        if expected_hash:
            actual_hash = _sha256(download_target)
            if actual_hash.lower() != expected_hash.lower():
                raise Exception(
                    f"hash mismatch for {os.path.basename(download_target)}: "
                    f"expected {expected_hash}, got {actual_hash}"
                )
        _pk_print("Extracting " + os.path.basename(download_target))
        with tarfile.open(download_target) as f:
            f.extractall(extract_dir)
    except Exception:
        if os.path.exists(extract_dir):
            shutil.rmtree(extract_dir, ignore_errors=True)
        raise
    finally:
        if os.path.exists(download_target):
            os.remove(download_target)

PK_MARKER_NAME = ".pk_package_hash"

def _read_marker(extract_dir):
    marker = os.path.join(extract_dir, PK_MARKER_NAME)
    if os.path.exists(marker):
        with open(marker) as f:
            return f.read().strip()
    return ""

def _write_marker(extract_dir, package_hash):
    if package_hash:
        with open(os.path.join(extract_dir, PK_MARKER_NAME), "w") as f:
            f.write(package_hash)

def download_pk_ifn(pk_version, pk_license, platform, target_folder):
    pk_package_name = f"PopcornFX-{pk_version}-{pk_license}-{platform}"
    extract_dir = target_folder + pk_package_name

    package = PK_PACKAGES.get(platform, {})
    package_id = package.get("id", "")
    package_hash = package.get("hash", "")

    if os.path.exists(extract_dir):
        if not package_hash or _read_marker(extract_dir) == package_hash:
            _pk_print("Using " + pk_package_name)
            return pk_package_name
        _pk_print(pk_package_name + " is out of date, re-downloading")
        shutil.rmtree(extract_dir, ignore_errors=True)

    pk_package_archive = pk_package_name + ".tar.xz"
    download_target = target_folder + pk_package_archive

    urls = []
    if package_id:
        urls.append(PK_DOWNLOAD_URL + package_id)
    urls.append(PK_FALLBACK_URL + pk_package_archive)

    for i, url in enumerate(urls):
        _pk_print("Downloading " + url)
        try:
            _download_validate_extract(url, download_target, extract_dir, package_hash)
            _write_marker(extract_dir, package_hash)
            return pk_package_name
        except Exception as e:
            if i == len(urls) - 1:
                raise
            _pk_print(f"Download failed ({e}), falling back")
