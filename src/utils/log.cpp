//------------------------------------------------------------------------------
// Author: Pavel Karneliuk
// Description: Reentrant logger implementation
// Copyright (c) 2013 EPAM Systems
//------------------------------------------------------------------------------
/*
    This file is part of Nfstrace.

    Nfstrace is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2 of the License.

    Nfstrace is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Nfstrace.  If not, see <http://www.gnu.org/licenses/>.
*/
//------------------------------------------------------------------------------
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <system_error>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils/log.h"
#include "utils/out.h"
//------------------------------------------------------------------------------
/*  http://www.unix.org/whitepapers/reentrant.html
    The POSIX.1 and C-language functions that operate on character streams
    (represented by pointers to objects of type FILE) are required by POSIX.1c
    to be implemented in such a way that reentrancy is achieved
    (see ISO/IEC 9945:1-1996, §8.2). This requirement has a drawback; it imposes
    substantial performance penalties because of the synchronization that must
    be built into the implementations of the functions for the sake of reentrancy.
*/
//------------------------------------------------------------------------------
namespace NST
{
namespace utils
{

static FILE* log_file {::stderr};
static bool  own_file {false};

namespace // unnanmed
{

static FILE* try_open(const std::string& file_name)
{
    FILE* file = fopen(file_name.c_str(), "a+");
    if(file == nullptr)
    {
        throw std::system_error{errno, std::system_category(),
                               {"Error in opening file: " + file_name}};
    }
    chmod(file_name.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(flock(fileno(file), LOCK_EX | LOCK_NB))
    {
        fclose(file);
        throw std::system_error{errno, std::system_category(),
                               {"Log file already locked: " + file_name}};
    }
    time_t now = time(NULL);
    fprintf(file, "--------------------------------------------------------------------------\n");
    fprintf(file, "--------------------------------------------------------------------------\n");
    fprintf(file, "Nfstrace log: PID = %lu %s", static_cast<unsigned long>(getpid()), ctime(&now));
    fprintf(file, "--------------------------------------------------------------------------\n");
    return file;
}

} // namespace unnamed

Log::Global::Global(const std::string& path)
    : log_file_path {path}
{
    if(own_file)
    {
        throw std::runtime_error{"Global Logger already have been created."};
    }

    if(!log_file_path.empty())
    {
        struct stat st;
        bool exists = stat(log_file_path.c_str(), &st) == 0 ? true : false;

        if(!exists && log_file_path.back() == '/')
        {
            throw std::system_error{errno, std::system_category(),
                                   {"Error accessing directory: " + log_file_path}};
        }

        if(exists && S_ISDIR(st.st_mode))
        {
            throw std::system_error{errno, std::system_category(),
                                   {"Incorrect log file path: " + log_file_path + " - it is a directory! Please set correct path to log."}};
        }
    }
    else
    {
        log_file_path = "nfstrace.log";
    }

    log_file = try_open(log_file_path);
    own_file = true;

    if(utils::Out message{})
    {
        message << "Log file: " << log_file_path;
    }
}

Log::Global::~Global()
{
    if(own_file)
    {
        flock(fileno(log_file), LOCK_UN);
        fclose(log_file);
        own_file = false;
        log_file = ::stderr;
    }
}

void Log::Global::reopen()
{
    if(!own_file || log_file == ::stderr || log_file == ::stdout || log_file == nullptr)
        return;
    FILE* temp = freopen(log_file_path.c_str(), "a+", log_file);
    if(temp == nullptr)
    {
        throw std::system_error{errno, std::system_category(),
                               {std::string{"Can't reopen file: "} + log_file_path}};
    }
    log_file = temp;
}

Log::Log()
: std::stringbuf {ios_base::out}
, std::ostream   {nullptr}
{
    std::stringbuf::setp(buffer, buffer+sizeof(buffer));
    std::ostream::init(static_cast<std::stringbuf*>(this));
    std::ostream::put('\n');
}

Log::~Log()
{
    size_t len = pptr() - pbase();
    fwrite(pbase(), len, 1, log_file);
}

void Log::message(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
}

void Log::flush()
{
    fflush(log_file);
}

} // namespace utils
} // namespace NST
//------------------------------------------------------------------------------
