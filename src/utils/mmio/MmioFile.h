#pragma once

#define _AMD64_
#include <minwindef.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace EGL3::Utils::Mmio {
	// Read/write memory mapped file
	class MmioFile {
	public:
		MmioFile(const fs::path& FilePath);

		MmioFile(const char* FilePath);

		MmioFile(const MmioFile&) = delete;
		MmioFile& operator=(const MmioFile&) = delete;

		~MmioFile();

		bool Valid() const;

		char* Get() const {
			return (char*)BaseAddress;
		}

		size_t Size() const {
			return SectionSize.QuadPart;
		}

		void EnsureSize(size_t Size);

		void Flush();

	private:
		HANDLE HProcess;
		HANDLE HSection;
		PVOID BaseAddress;
		LARGE_INTEGER SectionSize;
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
		HANDLE HProcess;
		HANDLE HSection;
		const PVOID BaseAddress;
		SIZE_T FileSize;
	};
}