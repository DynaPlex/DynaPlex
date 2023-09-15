#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>  // for std::thread::hardware_concurrency
#include <iomanip> // for std::setw, std::setfill
#include "dynaplex/systeminfo.h"
#include "dynaplex/error.h"
namespace fs = std::filesystem;

namespace DynaPlex {

    class SystemInfo::Impl {
    public:
        Impl(bool torchavailable, int32_t world_rank, int32_t world_size, std::function<void()> barrier_cb) : start_time_(std::chrono::steady_clock::now()),
            hardware_threads_(std::thread::hardware_concurrency()),
            world_rank_(world_rank),
            world_size_(world_size),
            barrier_callback_(barrier_cb) {

        }
        // Default copy constructor
        Impl(const Impl& other) = default;

        // Default copy assignment operator
        Impl& operator=(const Impl& other) = default;
        std::chrono::steady_clock::time_point start_time_;
        std::uint32_t hardware_threads_;
        std::uint32_t world_rank_;
        std::uint32_t world_size_;
        bool torchavailable;
        fs::path io_location_;
        std::function<void()> barrier_callback_;
    };

    void SystemInfo::AddBarrier() const {
        if (pimpl->barrier_callback_) {
            pimpl->barrier_callback_();
        }
    }
    SystemInfo::SystemInfo() = default;
    SystemInfo::SystemInfo(bool torchavailable, std::uint32_t worldRank, std::uint32_t worldSize, std::function<void()> barrier_cb)
        : pimpl(std::make_unique<Impl>(torchavailable, worldRank, worldSize,barrier_cb)) {
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
        fs::path subdir_path = pimpl->io_location_ / subdir;

        // Check if subdir_path exists
        if (!fs::exists(subdir_path)) {
            // If subdir_path doesn't exist, attempt to create it
            fs::create_directory(subdir_path);
        }
        else if (!fs::is_directory(subdir_path)) {
            // Handle the error: subdir_path exists but is not a directory
            throw std::runtime_error(subdir_path.string() + " exists but is not a directory.");
        }

        fs::path file_path = subdir_path / filename;
        return file_path.string();
    }

    std::string SystemInfo::filename(const std::string& filename) {
        fs::path file_path = pimpl->io_location_ / filename;
        return file_path.string();
    }

    bool SystemInfo::HasIODirectory() const {
        // If io_location_ is not set or doesn't point to a valid directory, return false.
        if (pimpl->io_location_.empty() || !fs::exists(pimpl->io_location_) || !fs::is_directory(pimpl->io_location_)) {
            return false;
        }

        // Otherwise, return true.
        return true;
    }


    void SystemInfo::SetIOLocation(const std::string& path, const std::string& dirname)
    {
        fs::path base_path(path);
        fs::path combined_path = base_path / dirname;

        // Check if base_path is a valid existing directory
        if (!fs::exists(base_path) || !fs::is_directory(base_path))
        {
            // Handle the error: base_path is not a valid directory
            throw DynaPlex::Error(base_path.string() + " is not a valid directory.");
            return;
        }

        // Check if combined_path is a directory or can be created as one
        if (fs::exists(combined_path))
        {
            if (!fs::is_directory(combined_path))
            {
                // Handle the error: combined_path exists but is not a directory
                throw DynaPlex::Error(combined_path.string() + " exists but is not a directory.");
                return;
            }
        }
        else
        {
            // If combined_path doesn't exist, create it
            fs::create_directory(combined_path);
        }

        pimpl->io_location_ = combined_path;
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
