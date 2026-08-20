//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/resource_loader.hpp"

#include "integration/pk_sdk.h"

#include <pk_kernel/include/kr_file_controller.h>

using namespace godot;

class PKFileSystemController;

class PKFileStream : public CFileStream {
public:
	virtual ~PKFileStream();

	static PKFileStream *open(PKFileSystemController *p_controller, PFilePack p_pack, const CString &p_path, IFileSystem::EAccessPolicy p_mode);

	virtual u64 Read(void *p_target_buffer, u64 p_byte_count) override;
	virtual u64 Write(const void *p_source_buffer, u64 p_byte_count) override;
	virtual bool Seek(s64 p_offset, ESeekMode p_whence) override;
	virtual u64 Tell() const override;
	virtual bool Eof() const override;
	virtual bool Flush() override;
	virtual void Close() override;
	virtual u64 SizeInBytes() const override;
	virtual void Timestamps(SFileTimes &p_timestamps) override;

private:
	PKFileStream(
			PKFileSystemController *p_controller,
			PFilePack p_pack,
			const CString &p_path,
			IFileSystem::EAccessPolicy p_mode,
			Ref<FileAccess> p_file);

	Ref<FileAccess> file_access;
};
typedef PopcornFX::TRefPtr<PKFileStream> PPKFileStream;
typedef PopcornFX::TWeakPtr<PKFileStream> WPKFileStream;
typedef PopcornFX::TRefPtr<const PKFileStream> PCPKFileStream;
typedef PopcornFX::TWeakPtr<const PKFileStream> WCPKFileStream;

class PKFileSystemController : public CFileSystemBase {
public:
	PKFileSystemController() {}
	~PKFileSystemController() {}

	virtual PFileStream OpenStream(const CString &p_path, IFileSystem::EAccessPolicy p_mode, bool p_path_not_virtual) override;
	virtual bool Exists(const CString &p_path, bool p_path_not_virtual = false) override;
	virtual bool FileDelete(const CString &p_path, bool p_path_not_virtual = false) override;
	virtual bool FileCopy(const CString &p_source_path, const CString &p_target_path, bool p_paths_not_virtual = false) override;
	virtual void GetDirectoryContents(char *p_dpath, char *p_virtual_path, uint32_t p_path_length, CFileDirectoryWalker *p_walker, const CFilePack *p_pack) override;
	virtual bool CreateDirectoryChainIFN(const CString &p_directory_path, bool p_path_not_virtual = false) override;
	virtual bool DirectoryDelete(const CString &p_path, bool p_path_not_virtual = false) override;
	virtual CString VirtualToPhysical(const CString &p_virtual_path, EAccessPolicy p_mode, PFilePack *r_pack = nullptr) override;

	static PKFileSystemController *get_default() {
		return dynamic_cast<PKFileSystemController *>(File::DefaultFileSystem());
	}

	String sanitize_path(const String &p_path, bool p_pk_virtual);
	String sanitize_path(const CString &p_path, bool p_pk_virtual);
	const String &get_source_pack_root_dir() const { return cached_source_pack_root_dir; }
	void set_source_pack_root_dir(const String &p_dir) { cached_source_pack_root_dir = p_dir; }

	template <typename T>
	Ref<T> load_resource_from_virtual_path(const String &p_path) {
		const String full_path = sanitize_path(p_path, true);

		Ref<T> loaded = ResourceLoader::get_singleton()->load(full_path);

		if (loaded == nullptr) {
			ERR_PRINT("Couldn't load resource : '" + full_path + "'.");
		}
		return loaded;
	}

private:
	String cached_source_pack_root_dir;
};
