#pragma once
#include <cstdint>
#include <memory> // for std::unique_ptr
#include <iostream>  // For std::cout
#include <string>
#include <functional>


namespace DynaPlex {

    class System {
    public: 
        System();
        System(bool TorchAvailable, std::uint32_t worldRank, std::uint32_t worldSize, std::function<void()> barrier_cb);
        ~System();

        System(const System&);  // Copy constructor
        System& operator=(const System&);  // Copy assignment operator

        /// returns whether the System has a valid IO directory defined. 
        bool HasIODirectory() const;
        /// hardwarethreads available for the process or algorithm that receives this system.
        std::uint32_t HardwareThreads() const;
        std::uint32_t WorldRank() const;
        std::uint32_t WorldSize() const;

        /// takes a path to a file (existing or not) and sets or replaces the file extension. 
        static std::string SetFileExtension(const std::string& filepath, const std::string& extension);

        /// removes file with the mentioned path. Throws if file does not exist. 
        void remove_file(const std::string& file_path);

        /// returns whether file exists: IOLocation()/filename.
        bool file_exists(const std::string& filename) const;

        /// returns whether file exists: IOLocation()/subdir/filename.
        bool file_exists(const std::string& subdir, const std::string& filename) const;

        /// returns whether file exists: IOLocation()/subdir/subsubdir/filename.
        bool file_exists(const std::string& subdir, const std::string& subsubdir, const std::string& filename) const;

        /// returns whether file exists: IOLocation()/subdir/subsubdir/subsubsubdir/filename.
        bool file_exists(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& filename) const;

        /// returns whether file exists: IOLocation()/subdir/subsubdir/subsubsubdir/subsubsubsubdir/filename.
        bool file_exists(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& subsubsubsubdir, const std::string& filename) const;


        /**
         *Creates IOLocation() / subdir / subsubdir / subsubsubdir / subsubsubsubdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / subsubdir / subsubsubdir / subsubsubsubdir / filename.
         */
        std::string filepath(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& subsubsubsubdir, const std::string& filename) const;


        /**
         *Creates IOLocation() / subdir / subsubdir /subsubsubdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / subsubdir  /subsubsubdir / filename.
         */
        std::string filepath(const std::string& subdir, const std::string& subsubdir, const std::string& subsubsubdir, const std::string& filename) const;


        /**
         *Creates IOLocation() / subdir / subsubdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / subsubdir / filename.
         */
        std::string filepath(const std::string& subdir, const std::string& subsubdir, const std::string& filename) const;

        /**
         *Creates IOLocation() / subdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / filename.
         */
        std::string filepath(const std::string& subdir, const std::string& filename) const;
        
        /**
         *Creates IOLocation() / subdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / filename.
         */
        std::string filepath(const std::string& filename) const;
        /// Returns path to input-output folder, i.e. to IO_DynaPlex folder. 
        std::string IOLocation() const;
        /// Elapsed time in ms. 
        std::int64_t ElapsedMS() const;
        /// Elapsed time (typeset like hh:mm:ss.ms) since object creation
        std::string Elapsed() const;

        /// typesets a timespan in ms like hh:mm:ss.ms 
        std::string Elapsed(std::int64_t timespan_ms) const;


        ///Sets the directory for IO. 
        void SetIOLocation(const std::string& path, const std::string& dirname);

        ///adds a MPI barrier, if applicable 
        void AddBarrier() const;

        /// if this process has world_rank 0, displays message on console. Otherwise, does nothing. 
        friend const System& operator<<(const System& sys, const std::string& msg);

        /// if this process has world_rank 0, puts func to console. Otherwise, does nothing. 
        friend const System& operator<<(const System& sys, std::ostream& (*func)(std::ostream&));

        /// if this process has world_rank 0, puts value to console. Otherwise, does nothing.
        template<typename T>
        friend const System& operator<<(const System& sys, const T& value) {
            if (sys.WorldRank() == 0) {
                std::cout << value;
            }
            return sys;
        }
    private:

        std::string filepath(const std::initializer_list<std::string>& subdirs, const std::string& filename) const;


        class Impl;  // Forward declare the implementation class
        std::unique_ptr<Impl> pimpl;  // Pointer to the implementation
    };

} // namespace DynaPlex
