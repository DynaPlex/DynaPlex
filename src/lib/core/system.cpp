#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>  // for std::thread::hardware_concurrency
#include <iomanip> // for std::setw, std::setfill
#include "dynaplex/system.h"
#include "dynaplex/error.h"
namespace fs = std::filesystem;

namespace DynaPlex {

    class System::Impl {
    public:
        Impl(bool torchavailable, int32_t world_rank, int32_t world_size, std::function<void()> barrier_cb) : start_time_(std::chrono::steady_clock::now()),
            hardware_threads_(std::thread::hardware_concurrency()),
            world_rank_(world_rank),
            world_size_(world_size),
            barrier_callback_(barrier_cb) {

        }
        // Default copy constructor
        Impl(const Impl& other) = default;


      

        bool file_exists_impl(const std::initializer_list<std::string>& subdirs, const std::string& filename) const {
            fs::path file_path = io_location_;

            for (const auto& subdir : subdirs) {
                file_path /= subdir;
            }

            file_path /= filename;

            return fs::exists(file_path);
        }

        std::string filepath_impl(const std::vector<std::string>& subdirs, const std::string& filename) const {
            fs::path curr_path = io_location_;

            for (const auto& subdir : subdirs) {
                curr_path /= subdir;

                if (!fs::exists(curr_path)) {
                    fs::create_directory(curr_path);
                }
                else if (!fs::is_directory(curr_path)) {
                    throw std::runtime_error(curr_path.string() + " exists but is not a directory.");
                }
            }

            curr_path /= filename;
            return curr_path.string();
        }

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


   

    std::string System::SetFileExtension(const std::string& filepath, const std::string& extension) {
        fs::path p(filepath);
        p.replace_extension(extension);
        return p.string();
    }

    void System::AddBarrier() const {
        if (pimpl->barrier_callback_) {
            pimpl->barrier_callback_();
        }
    }
    System::System() = default;
    System::System(bool torchavailable, std::uint32_t worldRank, std::uint32_t worldSize, std::function<void()> barrier_cb)
        : pimpl(std::make_unique<Impl>(torchavailable, worldRank, worldSize,barrier_cb)) {
    }
    System::~System() = default;

    System::System(const System& other)
        : pimpl(std::make_unique<Impl>(*other.pimpl)) {}

    System& System::operator=(const System& other) {
        if (this != &other) {
            pimpl = std::make_unique<Impl>(*other.pimpl);
        }
        return *this;
    }

   
    std::uint32_t System::HardwareThreads() const {
        return pimpl->hardware_threads_;
    }

    std::uint32_t System::WorldRank() const {
        return pimpl->world_rank_;
    }

    std::uint32_t System::WorldSize() const {
        return pimpl->world_size_;
    }

    void System::remove_file(const std::string& file_path)
    {
        if (!fs::remove(file_path))
        {
            throw DynaPlex::Error("System::remove_file - could not delete file.");
        }
    }


    bool System::file_exists(const std::string& filename) const {
        return pimpl->file_exists_impl({ }, filename);
    }

    bool System::file_exists(const std::string& subdir, const std::string& filename) const {
        return pimpl->file_exists_impl({ subdir }, filename);
    }

    bool System::file_exists(const std::string& subdir, const std::string& subsubdir, const std::string& filename) const {
        return pimpl->file_exists_impl({ subdir , subsubdir }, filename);
    }

    bool System::file_exists(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& filename) const {
        return pimpl->file_exists_impl({ subdir , subsubdir,subsubsubdir }, filename);
    }

    bool System::file_exists(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& subsubsubsubdir, const std::string& filename) const {
        return pimpl->file_exists_impl({ subdir , subsubdir,subsubsubdir, subsubsubsubdir }, filename);
    }

    std::string System::filepath(const std::vector<std::string>& subdirs, const std::string& filename) const {
        return pimpl->filepath_impl(subdirs, filename);
    }


    std::string System::filepath(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& subsubsubsubdir, const std::string& filename) const {
        return pimpl->filepath_impl({ subdir,subsubdir,subsubsubdir,subsubsubsubdir }, filename);
    }

    std::string System::filepath(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& filename) const {
        return pimpl->filepath_impl({ subdir,subsubdir,subsubsubdir }, filename);
    }

    std::string System::filepath(const std::string& subdir, const std::string& subsubdir, const std::string& filename) const {
        return pimpl->filepath_impl({subdir,subsubdir}, filename);
    }

    std::string System::filepath(const std::string& subdir, const std::string& filename) const {
        return pimpl->filepath_impl({ subdir }, filename);
    }

    std::string System::filepath(const std::string& filename) const {
        return pimpl->filepath_impl({}, filename);
    }

    bool System::HasIODirectory() const {
        // If io_location_ is not set or doesn't point to a valid directory, return false.
        if (pimpl->io_location_.empty() || !fs::exists(pimpl->io_location_) || !fs::is_directory(pimpl->io_location_)) {
            return false;
        }

        // Otherwise, return true.
        return true;
    }


    void System::SetIOLocation(const std::string& path, const std::string& dirname)
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

    std::string System::IOLocation() const {
        return pimpl->io_location_.string();
    }

    std::int64_t System::ElapsedMS() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - pimpl->start_time_);
        return duration.count();
    }

        
    std::string System::Elapsed(std::int64_t ms) const
    {
        auto total_seconds = ms / 1000;
        int days = total_seconds / (24 * 3600);
        total_seconds %= (24 * 3600);
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;
        int seconds = total_seconds % 60;

        int milliseconds = ms % 1000;

        std::ostringstream oss;

        if (days > 0) {
            oss << days << "-";
        }
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << seconds << "."
            << std::setw(3) << std::setfill('0') << milliseconds; 

        return oss.str();
    }

    

    std::string System::Elapsed() const {
        return this->Elapsed(ElapsedMS());
    }

   

    const System& operator<<(const System& sys, const std::string& msg) {
        if (sys.WorldRank() == 0) {
            std::cout << msg;
        }
        return sys;
    }

    const System& operator<<(const System& sys, std::ostream& (*func)(std::ostream&)) {
        if (sys.WorldRank() == 0) {
            std::cout << func;
        }
        return sys;
    }

} // namespace DynaPlex
