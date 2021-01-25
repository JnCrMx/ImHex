#include <hex/providers/provider.hpp>

namespace hex
{
    class ProcMemProvider : public prv::Provider {
	public:
        explicit ProcMemProvider(int pid, u64 lastAddress, u64 testAddress);
        ~ProcMemProvider() override;

        bool isAvailable() override;
        bool isReadable() override;
        bool isWritable() override;

        void read(u64 offset, void *buffer, size_t size) override;
        void write(u64 offset, const void *buffer, size_t size) override;

        void readRaw(u64 offset, void *buffer, size_t size) override;
        void writeRaw(u64 offset, const void *buffer, size_t size) override;
        size_t getActualSize() override;

        std::vector<std::pair<std::string, std::string>> getDataInformation() override;
	private:
        int m_pid;
        u64 m_lastAddress;
        bool m_readable, m_writable;
	};

}
