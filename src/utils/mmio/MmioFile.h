#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace EGL3::Utils::Mmio {
	typedef void* MM_HANDLE;
	typedef void* MM_PVOID;
	typedef long long MM_LARGE_INTEGER;
	typedef size_t MM_SIZE_T;

	// Read/write memory mapped file
	class MmioFile {
	public:
		MmioFile(const fs::path& FilePath);

		MmioFile(const char* FilePath);

		MmioFile(const MmioFile&) = delete;
		MmioFile& operator=(const MmioFile&) = delete;

		~MmioFile();

		bool IsValid() const;

		char* Get() const;

		size_t Size() const;

		bool IsValidOffset(size_t Offset) const;

		void EnsureSize(size_t Size);

		void Flush();

	private:
		MM_HANDLE HProcess;
		MM_HANDLE HSection;
		MM_PVOID BaseAddress;
		MM_LARGE_INTEGER SectionSize;
	};

	class MmioReadonlyFile {
	public:
		MmioReadonlyFile(const fs::path& FilePath);

		MmioReadonlyFile(const char* FilePath);

		MmioReadonlyFile(const MmioReadonlyFile&) = delete;
		MmioReadonlyFile& operator=(const MmioReadonlyFile&) = delete;

		~MmioReadonlyFile();

		bool Valid() const;

		const char* Get() const {
			return (const char*)BaseAddress;
		}

		size_t Size() const {
			return FileSize;
		}

	private:
		MM_HANDLE HProcess;
		MM_HANDLE HSection;
		const MM_PVOID BaseAddress;
		MM_SIZE_T FileSize;
	};
}