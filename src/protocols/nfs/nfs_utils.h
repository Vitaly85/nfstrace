//------------------------------------------------------------------------------
// Author: Alexey Costroma
// Description: Helpers for parsing NFS structures.
// Copyright (c) 2014 EPAM Systems
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
#ifndef NFS_UTILS_H
#define NFS_UTILS_H
//------------------------------------------------------------------------------
#include <ostream>
#include <iomanip>
#include <cstring>

#include "utils/out.h"
//------------------------------------------------------------------------------
#define NST_PUBLIC __attribute__ ((visibility("default")))
//------------------------------------------------------------------------------
namespace NST
{
namespace protocols
{
namespace NFS
{

inline bool out_all()
{
    using Out = NST::utils::Out;

    return Out::Global::get_level() == Out::Level::All;
}

namespace 
{
template <typename T>
struct Helper
{ 
    static void print_hex(std::ostream& out, T val)
    {
        out << "0x" << std::setfill('0') << std::setw(sizeof(T)/4) << std::hex << val
            << std::dec << std::setfill(' ');
    }
};
template <>
struct Helper<uint8_t>
{ 
    static void print_hex(std::ostream& out, char val)
    {
        out << "0x" << std::setfill('0') << std::setw(4) << std::hex << 
            static_cast<uint16_t>(val) << std::dec << std::setfill(' ');
    }
};
}

template <typename T>
void print_hex(std::ostream& out, T val)
{
    Helper<T>::print_hex(out, val);
} 

void print_hex64(std::ostream& out, uint64_t val);

void print_hex32(std::ostream& out, uint32_t val);

void print_hex16(std::ostream& out, uint16_t val);

void print_hex8(std::ostream& out, uint8_t val); 

void print_hex(std::ostream& out,
       const uint32_t* const val,
              const uint32_t len);

void print_hex(std::ostream& out,
           const char* const val,
              const uint32_t len);

extern "C"
NST_PUBLIC
void print_nfs_fh(std::ostream& out,
              const char* const val,
                 const uint32_t len);

} // namespace NFS
} // namespace protocols
} // namespace NST
//------------------------------------------------------------------------------
#endif//NFS_UTILS_H
//------------------------------------------------------------------------------
