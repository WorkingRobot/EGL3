#pragma once

#include <filesystem>
#include <functional>
#include <future>

namespace EGL3::Installer::Backend {
	enum class InstallState {
		INIT_PINNING,
		INIT_STREAMS,
		INIT_DOWNLOAD,
		DOWNLOAD,
		REGISTRY,
		SHORTCUT,
		DONE
	};

	class InstallManager {
	public:
        InstallManager(const std::filesystem::path& InstallPath, const std::function<void()>& UpdateCallback, const std::function<void(const std::string&)>& ErrorCallback);

        void Run();

		const std::filesystem::path& GetStartExe() const {
			return StartExe;
		}

		float GetProgress() const {
			return Progress;
		}

		InstallState GetState() const {
			return CurrentState;
		}

		const std::string& GetStateInfo() const {
			return StateInfo;
		}

	private:
        void UpdateState(InstallState State, float Progress, const std::string& StateInfo);

		static void CreateShortcut(const std::string& Target, const std::string& TargetArgs, const std::wstring& Output, const std::string& Description, int ShowMode, const std::string& CurrentDirectory, const std::string& IconFile, int IconIndex);

		std::future<void> RunTask;

		std::filesystem::path InstallLocation;
		InstallState CurrentState;
		std::string StateInfo;
		float Progress;
		std::function<void()> UpdateCallback;
		std::function<void(const std::string&)> ErrorCallback;

		std::filesystem::path StartExe;
	};
}