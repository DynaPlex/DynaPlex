#pragma once
#include <cstdint>
#include <memory> // for std::unique_ptr
#include <iostream>  // For std::cout and std::ostream
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
        /// hardwarethreads available to this thing. 
        std::uint32_t HardwareThreads() const;
        std::uint32_t WorldRank() const;
        std::uint32_t WorldSize() const;

        //file_exists methods still need implementation. 

        /// returns whether file exists: IOLocation()/filename.
        bool file_exists(const std::string& filename) const;

        /// returns whether file exists: IOLocation()/subdir/filename.
        bool file_exists(const std::string& subdir, const std::string& filename) const;

        /// returns whether file exists: IOLocation()/subdir/subsubdir/filename.
        bool file_exists(const std::string& subdir, const std::string& subsubdir, const std::string& filename) const;

        /**
         *Creates IOLocation() / subdir / subsubdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / subsubdir / filename.
         */
        std::string filename(const std::string& subdir, const std::string& subsubdir, const std::string& filename) const;

        /**
         *Creates IOLocation() / subdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / filename.
         */
        std::string filename(const std::string& subdir, const std::string& filename) const;
        
        /**
         *Creates IOLocation() / subdir if it does not exist.
         *Returns path to file (maybe non-existent) in IOLocation() / subdir / filename.
         */
        std::string filename(const std::string& filename) const;
        /// Returns path to input-output folder, i.e. to IO_DynaPlex folder. 
        std::string IOLocation() const;
        /// Elapsed time in ms. 
        std::int64_t ElapsedMS() const;
        /// Elapsed time (typeset like hh:mm:ss) since object creation
        std::string Elapsed() const;

        /// typesets a timespan in ms like hh:mm:ss. 
        std::string Elapsed(std::int64_t timespan_ms) const;


        ///Sets the directory for IO. 
        void SetIOLocation(const std::string& path, const std::string& dirname);

        ///adds a MPI barrier, if applicable 
        void AddBarrier() const;

        friend const System& operator<<(const System& sys, const std::string& msg);
        friend const System& operator<<(const System& sys, std::ostream& (*func)(std::ostream&));

    private:
        class Impl;  // Forward declare the implementation class
        std::unique_ptr<Impl> pimpl;  // Pointer to the implementation
    };

} // namespace DynaPlex
