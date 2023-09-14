#include "dynaplex/systeminfo.h"
#include <chrono>
#include <filesystem>
#include <thread>  // for std::thread::hardware_concurrency
#include <iomanip> // for std::setw, std::setfill

namespace DynaPlex {

    class SystemInfo::Impl {
    public:
        Impl(int32_t world_rank, int32_t world_size) : start_time_(std::chrono::steady_clock::now()),
            hardware_threads_(std::thread::hardware_concurrency()),
            world_rank_(world_rank),
            world_size_(world_size) {

        }
        // Default copy constructor
        Impl(const Impl& other) = default;

        // Default copy assignment operator
        Impl& operator=(const Impl& other) = default;

        std::uint32_t hardware_threads_;
        std::uint32_t world_rank_;
        std::uint32_t world_size_;
        std::filesystem::path io_location_;
        std::chrono::steady_clock::time_point start_time_;
    };

    SystemInfo::SystemInfo(std::uint32_t worldRank, std::uint32_t worldSize)
        : pimpl(std::make_unique<Impl>(worldRank, worldSize)) {
    }
    SystemInfo::~SystemInfo() = default;

    SystemInfo::SystemInfo(const SystemInfo& other)
        : pimpl(std::make_unique<Impl>(*other.pimpl)) {}

    SystemInfo& SystemInfo::operator=(const SystemInfo& other) {
        if (this != &other) {
            pimpl = std::make_unique<Impl>(*other.pimpl);
        }
        return *this;
    }

   
    std::uint32_t SystemInfo::HardwareThreads() const {
        return pimpl->hardware_threads_;
    }

    std::uint32_t SystemInfo::WorldRank() const {
        return pimpl->world_rank_;
    }

    std::uint32_t SystemInfo::WorldSize() const {
        return pimpl->world_size_;
    }
    std::string SystemInfo::filename(const std::string& subdir, const std::string& filename) {
        std::filesystem::path file_path = pimpl->io_location_ / subdir / filename;
        return file_path.string();
    }

    std::string SystemInfo::filename(const std::string& filename) {
        std::filesystem::path file_path = pimpl->io_location_ / filename;
        return file_path.string();
    }

    std::string SystemInfo::IOLocation() const {
        return pimpl->io_location_.string();
    }

    std::int64_t SystemInfo::ElapsedMS() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - pimpl->start_time_);
        return duration.count();
    }

    std::string SystemInfo::Elapsed() const {
        auto total_seconds = ElapsedMS() / 1000;
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;
        int seconds = total_seconds % 60;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << "::"
            << std::setw(2) << std::setfill('0') << minutes << "::"
            << std::setw(2) << std::setfill('0') << seconds;
        return oss.str();
    }



} // namespace DynaPlex
