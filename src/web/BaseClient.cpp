#include "BaseClient.h"

namespace EGL3::Web {
	BaseClient::~BaseClient() {
		Cancelled = true;

		// Prevents the client functions from running past the lifetime of itself
		std::unique_lock<std::mutex> lock(RunningFunctionMutex);
		RunningFunctionCV.wait(lock, [this] { return RunningFunctionCount <= 0; });

		// Now that we can ensure no requests are being run on our client, we should kill any existing tokens
		KillAuthentication();
	}
}