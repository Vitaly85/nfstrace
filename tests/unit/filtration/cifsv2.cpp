//------------------------------------------------------------------------------
// Author: Andrey Kuznetsov
// Description: Composite filtrator tests
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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "controller/running_status.h"
#include "filtration/filtration_processor.h"
#include "filtration/cifs_filtrator.h"
#include "filtration/filtrators.h"
#include "filtration/packet.h"
//------------------------------------------------------------------------------
using namespace NST::filtration;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::_;
//------------------------------------------------------------------------------

namespace
{

class Writer
{
public:

    class Collection
    {
        Collection* pImpl = nullptr;
    public:
        void set(Writer& w, NST::utils::NetworkSession* /*session_ptr*/)
        {
            pImpl = &w.collection;
        }

        virtual void reset()
        {
            if (pImpl)
            {
                pImpl->reset();
            }
        }

        virtual void push(PacketInfo& info, size_t size)
        {
            if (pImpl)
            {
                pImpl->push(info, size);
            }
        }

        virtual void skip_first(size_t size)
        {
            if (pImpl)
            {
                pImpl->skip_first(size);
            }
        }

        virtual void complete(PacketInfo& info)
        {
            if (pImpl)
            {
                pImpl->complete(info);
            }
        }

        operator bool()
        {
            return true;
        }

        virtual const uint8_t* data()
        {
            if (pImpl)
            {
                return pImpl->data();
            }
            return nullptr;
        }

        virtual size_t data_size()
        {
            if (pImpl)
            {
                return pImpl->data_size();
            }
            return 0;
        }

        virtual size_t capacity()
        {
            if (pImpl)
            {
                return pImpl->capacity();
            }
            return 0;
        }

        virtual void allocate()
        {
            if (pImpl)
            {
                pImpl->allocate();
            }
        }
    };
    class CollectionMock : public Collection
    {
    public:
        MOCK_METHOD0(reset, void());
        MOCK_METHOD2(push, void(PacketInfo&, size_t));
        MOCK_METHOD0(data, const uint8_t* ());
        MOCK_METHOD0(data_size, size_t());
        MOCK_METHOD0(capacity, size_t());
        MOCK_METHOD0(allocate, void());
        MOCK_METHOD1(skip_first, void(size_t));
        MOCK_METHOD1(complete, void(PacketInfo&));

    };

    CollectionMock collection;
};

}

TEST(Filtration, CIFSFiltratorReset)
{
    // Set conditions
    Writer mock;
    EXPECT_CALL(mock.collection, reset())
    .Times(1);

    CIFSFiltrator<Writer> f;
    f.set_writer(nullptr, &mock, 0);
    // Check
    f.reset();
}

TEST(Filtration, filtratorsResets)
{
    // Set conditions
    Writer mock;
    EXPECT_CALL(mock.collection, reset())
    .Times(2);

    Filtrators<Writer> f;
    f.set_writer(nullptr, &mock, 0);
    // Check
    f.reset();
}

TEST(Filtration, pushRPCheader)
{
    // Prepare data
    struct pcap_pkthdr header;
    header.caplen = header.len = 16;
    const uint8_t packet[] = {0x80, 0x00, 0x00, 0x84,
                              0xec, 0x8a, 0x42, 0xcb,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x02
                             };
    PacketInfo info(&header, packet, 0);

    Writer mock;
    EXPECT_CALL(mock.collection, data())
    .WillRepeatedly(Return(packet));
    EXPECT_CALL(mock.collection, data_size())
    .WillOnce(Return(0))
    .WillOnce(Return(0))
    .WillRepeatedly(Return(sizeof(packet)));
    EXPECT_CALL(mock.collection, capacity())
    .WillRepeatedly(Return(1000000));
    // Set conditions
    EXPECT_CALL(mock.collection, push(_, _))
    .Times(AtLeast(1));

    Filtrators<Writer> f;
    f.set_writer(nullptr, &mock, 0);
    // Check
    f.push(info);
}

TEST(Filtration, pushCIFSheader)
{
    // Prepare data
    struct pcap_pkthdr header;
    header.caplen = header.len = 16;
    const uint8_t packet[] = {0x00, 0x00, 0x00, 0x68,
                              0xfe, 0x53, 0x4d, 0x42,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00
                             };
    PacketInfo info(&header, packet, 0);

    Writer mock;
    EXPECT_CALL(mock.collection, data())
    .WillRepeatedly(Return(packet));
    EXPECT_CALL(mock.collection, data_size())
    .WillOnce(Return(0))
    .WillOnce(Return(0))
    .WillRepeatedly(Return(sizeof(packet)));
    EXPECT_CALL(mock.collection, capacity())
    .WillRepeatedly(Return(1000000));
    // Set conditions
    EXPECT_CALL(mock.collection, push(_, _))
    .Times(AtLeast(1));

    Filtrators<Writer> f;
    f.set_writer(nullptr, &mock, 0);
    // Check
    f.push(info);
}

//------------------------------------------------------------------------------
