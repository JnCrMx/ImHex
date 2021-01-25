#include <sys/uio.h>
#include <stdlib.h>
#include <iostream>
#include <cerrno>
#include <cstring>

#include "providers/procmem_provider.hpp"

namespace hex
{
    ProcMemProvider::ProcMemProvider(int pid, u64 lastAddress, u64 testAdress) : Provider(),
            m_pid(pid), m_lastAddress(lastAddress) {
        u8 localBuffer;

        struct iovec local[1];
        local[0].iov_base = &localBuffer;
        local[0].iov_len = 1;

        struct iovec remote[1];
        remote[0].iov_base = (void*) testAdress;
        remote[0].iov_len = 1;

        ssize_t nread = process_vm_readv(m_pid, local, 1, remote, 1, 0);
        if (nread < 0) {
            std::cerr << "Cannot read memory of process " << m_pid << ": " << std::strerror(errno) << std::endl;
            this->m_readable = false;
            this->m_writable = false;
            return;
        } else {
            this->m_readable = true;
        }

        ssize_t nwrote = process_vm_writev(m_pid, local, 1, remote, 1, 0);
        if (nwrote < 0) {
            std::cerr << "Cannot write memory of process " << m_pid << ": " << std::strerror(errno) << std::endl;
            this->m_writable = false;
        } else {
            this->m_writable = true;
        }
    }

    ProcMemProvider::~ProcMemProvider() {

    }

    bool ProcMemProvider::isAvailable() {
        return m_readable;
    }

    bool ProcMemProvider::isReadable() {
        return m_readable;
    }

    bool ProcMemProvider::isWritable() {
        return m_writable;
    }

    size_t ProcMemProvider::getActualSize() {
        return m_lastAddress;
    }

    std::vector<std::pair<std::string, std::string>> ProcMemProvider::getDataInformation() {
        std::vector<std::pair<std::string, std::string>> result;
        return result;
    }

    void ProcMemProvider::read(u64 offset, void *buffer, size_t size) {
        if ((offset + size) > this->getSize() || buffer == nullptr || size == 0)
            return;

        readRaw(offset, buffer, size);

        for (u64 i = 0; i < size; i++)
            if (this->m_patches.back().contains(offset + i))
                reinterpret_cast<u8*>(buffer)[i] = this->m_patches.back()[offset + PageSize * this->m_currPage + i];
    }

    void ProcMemProvider::write(u64 offset, const void *buffer, size_t size) {
        if ((offset + size) > this->getSize() || buffer == nullptr || size == 0)
            return;

        this->m_patches.push_back(this->m_patches.back());

        for (u64 i = 0; i < size; i++)
            this->m_patches.back()[offset + this->getBaseAddress() + i] = reinterpret_cast<const u8*>(buffer)[i];
    }

    void ProcMemProvider::readRaw(u64 offset, void *buffer, size_t size) {
        if ((offset + size) > this->getSize() || buffer == nullptr || size == 0)
            return;

        struct iovec local[1];
        local[0].iov_base = buffer;
        local[0].iov_len = size;

        struct iovec remote[1];
        remote[0].iov_base = (void*) (PageSize * this->m_currPage + offset);
        remote[0].iov_len = size;

        process_vm_readv(m_pid, local, 1, remote, 1, 0);
    }

    void ProcMemProvider::writeRaw(u64 offset, const void *buffer, size_t size) {
        if ((offset + size) > this->getSize() || buffer == nullptr || size == 0)
            return;

        struct iovec local[1];
        local[0].iov_base = (void*) buffer;
        local[0].iov_len = size;

        struct iovec remote[1];
        remote[0].iov_base = (void*) (PageSize * this->m_currPage + offset);
        remote[0].iov_len = size;

        process_vm_writev(m_pid, local, 1, remote, 1, 0);
    }
}
