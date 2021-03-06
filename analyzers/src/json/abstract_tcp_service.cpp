//------------------------------------------------------------------------------
// Author: Ilya Storozhilov
// Description: Abstract TCP-service class definition
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
#include <cstring>
#include <functional>
#include <system_error>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "abstract_tcp_service.h"
#include "utils/log.h"
//------------------------------------------------------------------------------

AbstractTcpService::AbstractTcpService(std::size_t workersAmount, int port, const std::string& host, int backlog) :
    _port{port},
    _host{host},
    _backlog{backlog},
    _isRunning{true},
    _threadPool{workersAmount},
    _listenerThread{},
    _serverSocket{0},
    _tasksQueue{},
    _tasksQueueMutex{},
    _tasksQueueCond{}
{
}

AbstractTcpService::~AbstractTcpService()
{
    // Disposing tasks which are still in queue
    while (!_tasksQueue.empty())
    {
        delete _tasksQueue.front();
        _tasksQueue.pop();
    }
}

void AbstractTcpService::start()
{
    _isRunning = true;
    // Setting up server TCP-socket
    _serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0)
    {
        throw std::system_error{errno, std::system_category(), "Opening server socket error"};
    }
    // Setting SO_REUSEADDR to true
    int reuseAddr = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) < 0)
    {
        throw std::system_error{errno, std::system_category(), "Setting SO_REUSEADDR socket option error"};
    }
    // Binding server socket to endpoint
    IpEndpoint endpoint{_host.c_str(), _port};
    if (bind(_serverSocket, endpoint.addrinfo()->ai_addr, endpoint.addrinfo()->ai_addrlen) != 0)
    {
        throw std::system_error{errno, std::system_category(), "Binding server socket error"};
    }
    // Converting socket to listening state
    if (listen(_serverSocket, _backlog) != 0)
    {
        throw std::system_error{errno, std::system_category(), "Converting socket to listening state error"};
    }
    // Creating threads for thread-pool
    for (auto & thr : _threadPool)
    {
        thr = std::thread{&AbstractTcpService::runWorker, this};
    }
    _listenerThread = std::thread{&AbstractTcpService::runListener, this};
}

void AbstractTcpService::stop()
{
    {
        // Waking up all awaiting threads
        std::unique_lock<std::mutex> lock{_tasksQueueMutex};
        _isRunning = false;
        _tasksQueueCond.notify_all();
    }
    // Joining to thread-pool threads and disposing them
    for (auto & thr : _threadPool)
    {
        thr.join();
    }
    // Joining to listener thread and closing server socket
    _listenerThread.join();
    close(_serverSocket);
}

void AbstractTcpService::runWorker()
{
    while (true)
    {
        std::unique_ptr<AbstractTask> pendingTask;
        {
            std::unique_lock<std::mutex> lock{_tasksQueueMutex};
            while (!pendingTask)
            {
                if (!_isRunning.load())
                {
                    return;
                }
                if (!_tasksQueue.empty())
                {
                    pendingTask.reset(_tasksQueue.front());
                    _tasksQueue.pop();
                }
                else
                {
                    _tasksQueueCond.wait(lock);
                }
            }
        }
        pendingTask->execute();
    }
}

void AbstractTcpService::runListener()
{
    while (_isRunning.load())
    {
        // Accepting incoming connection on socket
        struct timespec acceptDuration;
        fillDuration(acceptDuration);
        fd_set readDescriptorsSet;
        FD_ZERO(&readDescriptorsSet);
        FD_SET(_serverSocket, &readDescriptorsSet);
        int descriptorsCount = pselect(_serverSocket + 1, &readDescriptorsSet, NULL, NULL, &acceptDuration, NULL);
        if (descriptorsCount == 0)
        {
            // Timeout expired
            continue;
        }
        else if (descriptorsCount < 0)
        {
            std::system_error e{errno, std::system_category(), "Awaiting for incoming connection on server socket error"};
            LOG("ERROR: %s", e.what());
#ifdef __gnu_linux__
            // Several first pselect(2) calls cause "Interrupted system call" error (errno == EINTR)
            // if drop privileges option is used on Linux (see https://access.redhat.com/solutions/165483)
            if (errno == EINTR)
            {
                continue;
            }
#endif
            throw e;
        }
        // Extracting and returning pending connection
        int pendingSocketDescriptor = accept(_serverSocket, NULL, NULL);
        if (pendingSocketDescriptor < 0)
        {
            std::system_error e{errno, std::system_category(), "Accepting incoming connection on server socket error"};
            LOG("ERROR: %s", e.what());
            throw e;
        }
        // Create and enqueue task
        std::unique_ptr<AbstractTask> newTask{createTask(pendingSocketDescriptor)};
        {
            std::unique_lock<std::mutex> lock(_tasksQueueMutex);
            if (_tasksQueue.size() < MaxTasksQueueSize)
            {
                _tasksQueue.push(newTask.get());
                newTask.release();
                _tasksQueueCond.notify_one();
            }
            else
            {
                LOG("ERROR: TCP-service tasks queue overload has been detected")
            }
        }
    }
}

//------------------------------------------------------------------------------

AbstractTcpService::AbstractTask::AbstractTask(int socket) :
    _socket{socket}
{}

AbstractTcpService::AbstractTask::~AbstractTask()
{
    close(_socket);
}

//------------------------------------------------------------------------------
