//------------------------------------------------------------------------------
// Author: Ilya Storozhilov
// Description: TCP-endpoint class declaration
// Copyright (c) 2013-2014 EPAM Systems
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
#ifndef IP_ENDPOINT_H
#define IP_ENDPOINT_H
//------------------------------------------------------------------------------
#include <string>

#include <netdb.h>
//------------------------------------------------------------------------------
//! IP-endpoint (host:port) helper class to use in socket operations
class IpEndpoint
{
public:
    //! Loopback address name
    static constexpr const char* LoopbackAddress = "localhost";
    //! Wildcard address name
    static constexpr const char* WildcardAddress = "*";

    IpEndpoint() = delete;
    //! Constructs TCP-endpoint
    /*!
     * \param host Hostname or IP-address of the endpoint
     * \param port TCP-port
     * \param hostAsAddress Consider host as IP-address flag
     */
    IpEndpoint(const std::string& host, int port, bool hostAsAddress = false);
    //! Destructs TCP-endpoint
    ~IpEndpoint();

    //! Returns a pointer to 'struct addrinfo' structure for TCP-endpoint
    struct addrinfo* addrinfo()
    {
        return _addrinfo;
    }
private:
    struct addrinfo* _addrinfo;
};
//------------------------------------------------------------------------------
#endif//IP_ENDPOINT_H
//------------------------------------------------------------------------------
