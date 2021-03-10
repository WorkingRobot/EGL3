#pragma once
#include "consts.h"

#ifdef USE_LZ4_HC
#include "lz4hc.h"

#define LZ4_STREAM LZ4_streamHC_t
#define LZ4_CREATE_STREAM LZ4_createStreamHC
#define LZ4_CLOSE_STREAM LZ4_freeStreamHC
#define LZ4_WRITE_STREAM(streamPtr, src, dst, srcSize) LZ4_compress_HC_continue(streamPtr, src, dst, srcSize, LZ4_COMPRESSBOUND(srcSize))
#else
#include "lz4.h"

#define LZ4_STREAM LZ4_stream_t
#define LZ4_CREATE_STREAM LZ4_createStream
#define LZ4_CLOSE_STREAM LZ4_freeStream
#define LZ4_WRITE_STREAM(streamPtr, src, dst, srcSize) LZ4_compress_fast_continue(streamPtr, src, dst, srcSize, LZ4_COMPRESSBOUND(srcSize), 1)
#endif

#define MESSAGE_MAX_BYTES FILE_BUF_SIZE
#define RING_BUFFER_BYTES 1024 * 256 + MESSAGE_MAX_BYTES