#pragma once

#include <mbedtls/sha1.h>

namespace EGL3::Utils {
	class SHA1Builder {
	public:
		SHA1Builder();

		~SHA1Builder();

		void Update(const char* Input, size_t InputSize);

		void Finish(char Out[20]);

		bool HasError() const;

		int GetError() const;

	private:
		void CheckError(int ErrorCode);

		mbedtls_sha1_context Ctx;
		int ErrorCode;
	};
}
