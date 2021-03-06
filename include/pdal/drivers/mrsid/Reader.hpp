/******************************************************************************
* Copyright (c) 2011, Kirk McKelvey <kirkoman@gmail.com>
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#ifndef INCLUDED_DRIVERS_MRSID_READER_HPP
#define INCLUDED_DRIVERS_MRSID_READER_HPP

#include <string>
#include <pdal/Stage.hpp>
#include "lidar/MG4PointReader.h"

namespace LizardTech
{
class MG4PointReader;
}

namespace pdal
{
namespace drivers
{
namespace mrsid
{

class PDAL_DLL Reader : public pdal::Stage
{
public:
    Reader(const char *);

    const std::string& getDescription() const;
    const std::string& getName() const;

    void seekToPoint(boost::uint64_t pointNum);

    bool supportsIterator (StageIteratorType t) 
    {   
        if (t == StageIterator_Sequential ) return true;
        if (t == StageIterator_Random ) return true;
        if (t == StageIterator_Block ) return true;
        return false;
    }

    
protected:
    boost::uint32_t readBuffer(PointBuffer&);

private:
    Reader& operator=(const Reader&); // not implemented
    Reader(const Reader&); // not implemented
    LizardTech::MG4PointReader *m_reader;
};

}
}
} // end namespaces

#endif // INCLUDED_DRIVERS_MRSID_READER_HPP
