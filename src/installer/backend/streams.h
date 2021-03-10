#pragma once

#include <filesystem>

#include "use_lz4.h"

namespace fs = std::filesystem;

struct Header {
    std::string StartupExe; // -6
    std::string UninstallString; // -5
    std::string ModifyPath; // -4
    std::string HelpLink; // -3
    std::string AboutLink; // -2
    std::string PatchNotesLink; // -1
    int InstallSize; // added before decompression
};

class ArchiveFileStream {
public:
    ArchiveFileStream(fs::path outputPath, Header* header, std::function<void(const char* error)> error_callback) {
        path = outputPath;
        this->header = header;
        this->error_callback = error_callback;
        filename = std::string();
        state = -6;

        fileBuf = (char*)malloc(UNPACK_FILE_BUF_SIZE);
    }

    void recieve(const char* data, int length) {
        stream.insert(stream.end(), data, data + length);
        return read_queue();
    }

    void read_buf(char* dst, int length) {
        memcpy(dst, stream.data(), length);
        stream.erase(stream.begin(), stream.begin() + length);
    }

    bool read_byte(char* dest) {
        if (stream.size() <= 0) return false;
        *dest = stream.front();
        stream.erase(stream.begin());
        return true;
    }

    bool ensure_size(int size) {
        return stream.size() >= size;
    }

    ~ArchiveFileStream() {
        free(fileBuf);
        if (file) {
            fclose(file);
        }
    }

    std::string filename; // public for the progress bar information
private:
    void read_queue() {
        while (1) {
            switch (state)
            {
                // Header
            case -6:
                if (read_string(&header->StartupExe)) {
                    ++state;
                }
                else {
                    return;
                }
            case -5:
                if (read_string(&header->UninstallString)) {
                    ++state;
                }
                else {
                    return;
                }
            case -4:
                if (read_string(&header->ModifyPath)) {
                    ++state;
                }
                else {
                    return;
                }
            case -3:
                if (read_string(&header->HelpLink)) {
                    ++state;
                }
                else {
                    return;
                }
            case -2:
                if (read_string(&header->AboutLink)) {
                    ++state;
                }
                else {
                    return;
                }
            case -1:
                if (read_string(&header->PatchNotesLink)) {
                    ++state;
                }
                else {
                    return;
                }
                // Files
            case 0:
                if (read_string(&filename)) {
                    ++state;
                }
                else {
                    return;
                }
            case 1:
                if (ensure_size(sizeof(int)))
                {
                    read_buf((char*)&size, sizeof(int));
                    auto file_path = path / filename;
                    fs::create_directories(file_path.parent_path());
                    file = fopen(file_path.string().c_str(), "wb");
                    if (!file) {
                        char* buf = new char[100];
                        sprintf(buf, "Could not open file for installation (%s)", file_path.string().c_str());
                        error_callback(buf);
                        return;
                    }
                    ++state;
                }
                else {
                    return;
                }
            case 2:
                int read_amt = size > UNPACK_FILE_BUF_SIZE ? UNPACK_FILE_BUF_SIZE : size;
                while (ensure_size(read_amt)) {
                    read_buf(fileBuf, read_amt);
                    fwrite(fileBuf, 1, read_amt, file);
                    size -= read_amt;
                    if (!size) {
                        fclose(file);
                        file = nullptr;
                        filename.clear();
                        state = 0;
                        break;
                    }
                    read_amt = size > UNPACK_FILE_BUF_SIZE ? UNPACK_FILE_BUF_SIZE : size;
                }
                return;
            }
        }
    }

    // true if read completely, continue on
    // false if not enough in buffer
    bool read_string(std::string* string) {
        char character;
        while (read_byte(&character)) {
            if (!character) { // \0 character
                return true;
            }
            *string += character;
        }
        return false;
    }

    fs::path path;
    Header* header;
    std::function<void(const char* error)> error_callback;
    std::vector<char> stream;

    int size;
    FILE* file;
    char* fileBuf;
    char state;
};

class LZ4_DecompressionStream {
public:
    LZ4_DecompressionStream(ArchiveFileStream* callback) {
        lz4 = LZ4_createStreamDecode();
        outputStream = callback;

        cmpBuf = (char*)malloc(LZ4_COMPRESSBOUND(MESSAGE_MAX_BYTES));
        cmpBytes = 0;
        decBuf = (char*)malloc(RING_BUFFER_BYTES);
        decOffset = 0;
    }

    ~LZ4_DecompressionStream() {
        LZ4_freeStreamDecode(lz4);
        free(decBuf);
    }

    void recieve(const char* data, size_t length) {
        cmpStream.insert(cmpStream.end(), data, data + length);
        while (1) {
            if (cmpBytes) {
                if (cmpStream.size() >= cmpBytes) {
                    memcpy(cmpBuf, cmpStream.data(), cmpBytes);
                    cmpStream.erase(cmpStream.begin(), cmpStream.begin() + cmpBytes);
                    decompress();
                    cmpBytes = 0;
                }
                else {
                    break;
                }
            }
            else {
                if (cmpStream.size() > 4) {
                    memcpy(&cmpBytes, cmpStream.data(), 4);
                    cmpStream.erase(cmpStream.begin(), cmpStream.begin() + 4);
                }
                else {
                    break;
                }
            }
        }
    }

private:
    void decompress() {
        char* const decPtr = &decBuf[decOffset];
        const int decBytes = LZ4_decompress_safe_continue(lz4, cmpBuf, decPtr, cmpBytes, MESSAGE_MAX_BYTES);
        if (decBytes <= 0) return;
        decOffset += decBytes;
        outputStream->recieve(decPtr, decBytes);

        // Wraparound the ringbuffer offset
        if (decOffset >= RING_BUFFER_BYTES - MESSAGE_MAX_BYTES) decOffset = 0;
    }

    LZ4_streamDecode_t* lz4;
    ArchiveFileStream* outputStream;
    std::vector<char> cmpStream;

    char* cmpBuf;
    int cmpBytes;

    char* decBuf;
    int decOffset;
};