#pragma once

#include <semaphore>

namespace EGL3::Utils {
	template<class Sema>
	class SemaphoreGuard {
	public:
		explicit SemaphoreGuard(Sema& Semaphore) : Semaphore(Semaphore) {
			Semaphore.acquire();
		}

		~SemaphoreGuard() noexcept {
			Semaphore.release();
		}

		SemaphoreGuard(const SemaphoreGuard&) = delete;
		SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;

	private:
		Sema& Semaphore;
	};
}