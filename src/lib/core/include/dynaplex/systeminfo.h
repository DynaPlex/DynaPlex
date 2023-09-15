#include <cstdint>
#include <memory> // for std::unique_ptr
#include <string>
#include <functional>


namespace DynaPlex {

    class SystemInfo {
    public: 
        SystemInfo();
        SystemInfo(bool TorchAvailable, std::uint32_t worldRank, std::uint32_t worldSize, std::function<void()> barrier_cb);
        ~SystemInfo();

        SystemInfo(const SystemInfo&);  // Copy constructor
        SystemInfo& operator=(const SystemInfo&);  // Copy assignment operator

        /// returns whether the SystemInfo has a valid IO directory defined. 
        bool HasIODirectory() const;
        /// hardwarethreads available to this thing. 
        std::uint32_t HardwareThreads() const;
        std::uint32_t WorldRank() const;
        std::uint32_t WorldSize() const;
        /// Returns path to file in subdirectory of folder, either existing or new. 
        std::string filename(const std::string& subdir, const std::string& filename);
        /// Returns path to file in folder, either existing or new. 
        std::string filename(const std::string& filename);
        /// Returns path to input-output folder. 
        std::string IOLocation() const;
        /// Elapsed time in ms. 
        std::int64_t ElapsedMS() const;
        /// Elapsed time (typeset like hh::mm::ss) since object creation
        std::string Elapsed() const;
        ///Sets the directory for IO. 
        void SetIOLocation(const std::string& path, const std::string& dirname);

        ///adds a MPI barrier, if applicable 
        void AddBarrier() const;
    private:
        class Impl;  // Forward declare the implementation class
        std::unique_ptr<Impl> pimpl;  // Pointer to the implementation
    };

} // namespace DynaPlex
