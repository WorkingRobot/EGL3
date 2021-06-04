#include "interface.h"

AppendingFile::AppendingFile() :
    position(0)
{

}

int64_t AppendingFile::seek(int64_t offset, int whence) {
    switch (whence)
    {
    case SEEK_SET:
        return position = offset;
    case SEEK_CUR:
        return position += offset;
    case SEEK_END:
        return position = size + offset;
    default:
        return position;
    }
}

size_t AppendingFile::tell() {
    return position;
}

size_t AppendingFile::pread(uint8_t* buf, size_t size, int64_t offset) {
    size_t bytes_left = size;

    size_t cluster_idx = offset / 4096;
    size_t cluster_off = offset & 4095;
    while (bytes_left) {
        int64_t read_amt = std::min(4096 - cluster_off, bytes_left);
        memcpy(buf, get_cluster(cluster_idx) + cluster_off, read_amt);
        buf += read_amt;
        cluster_off = 0;
        bytes_left -= read_amt;
        cluster_idx++;
    }

    return size;
}

bool is_cluster_00(const uint8_t* b) {
    for (int n = 0; n < 4096; ++n) {
        if (b[n] != 0x00) {
            return false;
        }
    }
    return true;
}

size_t AppendingFile::pwrite(const uint8_t* buf, size_t size, int64_t offset) {
    size_t bytes_left = size;

    size_t cluster_idx = offset >> 12; // divide by 4096
    size_t cluster_off = offset & 4095;
    while (bytes_left) {
        int64_t read_amt = std::min(4096 - cluster_off, bytes_left);
        if (read_amt != 4096) {
            memcpy(get_cluster(cluster_idx) + cluster_off, buf, read_amt);
        }
        // it's 0 by default, don't do anything
        else if (!is_cluster_00(buf)) {
            memcpy(get_cluster(cluster_idx) + cluster_off, buf, read_amt);
        }
        buf += read_amt;
        cluster_off = 0;
        bytes_left -= read_amt;
        cluster_idx++;
    }

    return size;
}

size_t AppendingFile::read(uint8_t* buf, size_t size) {
    size_t ret = pread(buf, size, tell());
    if (ret != -1)
        seek(ret, SEEK_CUR);
    return ret;
}

size_t AppendingFile::write(const uint8_t* buf, size_t size) {
    size_t ret = pwrite(buf, size, tell());
    if (ret != -1)
        seek(ret, SEEK_CUR);
    return ret;
}

uint8_t* AppendingFile::try_get_cluster(int64_t cluster_idx) {
    auto search = data.find(cluster_idx);
    if (search != data.end()) {
        return search->second;
    }
    
    return NULL;
}

uint8_t* AppendingFile::get_cluster(int64_t cluster_idx) {
    auto search = data.find(cluster_idx);
    if (search != data.end()) {
        return search->second;
    }
    auto ret = (uint8_t*)calloc(1, 4096);
    data.emplace(cluster_idx, ret);

    return ret;
}

const std::unordered_map<uint64_t, uint8_t*>& AppendingFile::get_data() const
{
    return data;
}
