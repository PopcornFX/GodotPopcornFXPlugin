//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_file_system_controller.h"

#include "godot_cpp/classes/dir_access.hpp"

#include "integration/pk_plugin.h"

#include <pk_kernel/include/kr_containers_array.h>

using namespace godot;

PKFileStream::~PKFileStream() {
	Close();
}

u64 PKFileStream::Read(void *p_target_buffer, u64 p_byte_count) {
	return file_access->get_buffer((uint8_t *)p_target_buffer, p_byte_count);
}

u64 PKFileStream::Write(const void *p_source_buffer, u64 p_byte_count) {
	file_access->store_buffer((uint8_t *)p_source_buffer, p_byte_count);
	return p_byte_count;
}

bool PKFileStream::Seek(s64 p_offset, ESeekMode p_whence) {
	if (p_whence == CFileStream::SeekSet) {
		file_access->seek(p_offset);
	} else if (p_whence == CFileStream::SeekCur) {
		file_access->seek(file_access->get_position() + p_offset);
	} else {
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	return true;
}

u64 PKFileStream::Tell() const {
	return file_access->get_position();
}

bool PKFileStream::Eof() const {
	return file_access->eof_reached();
}

bool PKFileStream::Flush() {
	file_access->flush();
	return true;
}

void PKFileStream::Close() {
	file_access->close();
}

u64 PKFileStream::SizeInBytes() const {
	return file_access->get_length();
}

void PKFileStream::Timestamps(SFileTimes &p_timestamps) {
	(void)p_timestamps;
	//FileAccess::get_modified_time(m_File->get_path());
	PK_ASSERT_NOT_REACHED();
}

PKFileStream::PKFileStream(
		PKFileSystemController *p_controller,
		PFilePack p_pack,
		const CString &p_path,
		IFileSystem::EAccessPolicy p_mode,
		Ref<FileAccess> p_file_access) :
		CFileStream(p_controller, p_pack, p_path, p_mode), file_access(p_file_access) {
}

constexpr FileAccess::ModeFlags access_policy_to_godot[] = {
	FileAccess::READ, //	Access_Read = 0,
	FileAccess::READ_WRITE, //	Access_ReadWrite,
	FileAccess::WRITE, //	Access_WriteCreate,
	FileAccess::WRITE_READ, //	Access_ReadWriteCreate,
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(access_policy_to_godot) == IFileSystem::__MaxAccessPolicies);

PKFileStream *PKFileStream::open(PKFileSystemController *p_controller, PFilePack p_pack, const CString &p_path, IFileSystem::EAccessPolicy p_mode) {
	PKFileStream *stream = nullptr;
	CString raw_path = p_path;
	Ref<FileAccess> file = nullptr;

	if (p_path.StartsWith("res://")) {
		file = FileAccess::open(raw_path.Data(), access_policy_to_godot[p_mode]);
	} else if (p_path.StartsWith("res:/")) {
		raw_path = "res://" + CString::Extract(raw_path.Data(), 5, raw_path.Length());
		file = FileAccess::open(raw_path.Data(), access_policy_to_godot[p_mode]);
	}

	if (file != nullptr && file->is_open()) {
		stream = PK_NEW(PKFileStream(p_controller, p_pack, p_path, p_mode, file));
	} else {
		CLog::Log(PK_ERROR, "PKFileStream: Open: cannot open '%s'", raw_path.Data());
	}
	return stream;
}

// ----------------------------------------------------------------------------
//
// FileSystemController_Godot
//
PFileStream PKFileSystemController::OpenStream(const CString &p_path, IFileSystem::EAccessPolicy p_mode, bool p_path_not_virtual) {
	PFilePack pack;
	const CString real_path = p_path_not_virtual ? p_path : VirtualToPhysical(p_path, p_mode, &pack);
	if (!real_path.Empty()) {
		return PKFileStream::open(this, pack, real_path, p_mode);
	}
	return nullptr;
}

bool PKFileSystemController::Exists(const CString &p_path, bool p_path_not_virtual /*= false*/) {
	if (p_path.Empty()) {
		return false;
	}

	if (!p_path_not_virtual) {
		return !VirtualToPhysical(p_path, IFileSystem::Access_Read).Empty();
	}
	if (p_path.StartsWith("res://")) {
		return FileAccess::file_exists(p_path.Data());
	}
	if (p_path.StartsWith("res:/")) {
		const CString real_path = "res://" + CString::Extract(p_path.Data(), 5, p_path.Length());
		return FileAccess::file_exists(real_path.Data());
	}
	return false;
}

bool PKFileSystemController::FileDelete(const CString &p_path, bool p_path_not_virtual /*= false*/) {
	(void)p_path;
	(void)p_path_not_virtual;
	PK_ASSERT_NOT_REACHED();
	return false;
}

bool PKFileSystemController::FileCopy(const CString &p_source_path, const CString &p_target_path, bool p_paths_not_virtual /*= false*/) {
	(void)p_source_path;
	(void)p_target_path;
	(void)p_paths_not_virtual;
	PK_ASSERT_NOT_REACHED();
	return false;
}

void PKFileSystemController::GetDirectoryContents(char *p_dpath, char *p_virtual_path, uint32_t p_path_length, CFileDirectoryWalker *p_walker, const CFilePack *p_pack) {
	(void)p_dpath;
	(void)p_virtual_path;
	(void)p_path_length;
	(void)p_walker;
	(void)p_pack;
	PK_ASSERT_NOT_REACHED();
	return;
}

bool PKFileSystemController::CreateDirectoryChainIFN(const CString &p_directory_path, bool p_path_not_virtual /*= false*/) {
	CString path = p_directory_path;
	if (!p_path_not_virtual || path.StartsWith("res://")) {
		if (path.StartsWith("res://")) {
			path = CString::Extract(path.Data(), 6, path.Length());
		}

		CFilePath::Purify(path);

		TArray<CString> directory_names;
		path.Split('/', directory_names);
		String reconstructed_path = "res:/";

		const Ref<DirAccess> dir = DirAccess::open("res://");
		for (int i = 0; i < directory_names.Count(); ++i) {
			reconstructed_path += "/";
			reconstructed_path += directory_names[i].Data();

			if (!dir->dir_exists(reconstructed_path)) {
				if (dir->make_dir(reconstructed_path) != OK) {
					CLog::Log(PK_ERROR, "CreateDirectoryChainIFN: Failed to create directory %s", reconstructed_path.utf8().ptr());
					return false;
				}
			}
		}
	} else {
		CFilePath::Purify(path);
		String reconstructed_path = "";
		reconstructed_path += path.Data();
		if (DirAccess::make_dir_recursive_absolute(reconstructed_path) != OK) {
			CLog::Log(PK_ERROR, "CreateDirectoryChainIFN: Failed to create directory %s", reconstructed_path.utf8().ptr());
			return false;
		}
	}
	return true;
}

bool PKFileSystemController::DirectoryDelete(const CString &p_path, bool p_path_not_virtual /*= false*/) {
	(void)p_path;
	(void)p_path_not_virtual;
	PK_ASSERT_NOT_REACHED();
	return false;
}

CString PKFileSystemController::VirtualToPhysical(const CString &p_virtual_path, EAccessPolicy p_mode, PFilePack *r_pack /*= nullptr*/) {
	CString clean_path = p_virtual_path;
	CFilePath::Purify(clean_path);
	const CString virtual_to_physical = VirtualToPhysical_Pure(clean_path, p_mode, r_pack);
	return to_pk(sanitize_path(virtual_to_physical, false));
}

String PKFileSystemController::sanitize_path(const String &p_path, bool p_pk_virtual) {
	if (p_path.begins_with("res:/") && !p_path.begins_with("res://")) { // "res://" path has been purified by PK, un-purify it
		return "res://" + p_path.right(-5);
	} else if (p_pk_virtual) {
		return cached_source_pack_root_dir.path_join(p_path);
	}
	return p_path;
}

String PKFileSystemController::sanitize_path(const CString &p_path, bool p_pk_virtual) {
	return sanitize_path(to_gd(p_path), p_pk_virtual);
}
