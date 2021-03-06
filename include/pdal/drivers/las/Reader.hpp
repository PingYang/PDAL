/******************************************************************************
* Copyright (c) 2011, Michael P. Gerlek (mpg@flaxen.com)
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

#ifndef INCLUDED_DRIVERS_LAS_READER_HPP
#define INCLUDED_DRIVERS_LAS_READER_HPP

#include <pdal/pdal.hpp>

#include <pdal/Stage.hpp>
#include <pdal/drivers/las/Header.hpp>
#include <pdal/drivers/las/ReaderBase.hpp>

class LASunzipper;

namespace pdal
{
    class PointBuffer;
}

namespace pdal { namespace drivers { namespace las {

class LasHeader;
class ZipPoint;

class PDAL_DLL Reader : public ReaderBase
{
public:
    SET_STAGE_NAME("drivers.las.reader", "Las Reader")

    Reader(const Options&);
    Reader(const std::string& filename);
    
    virtual void initialize();
    virtual const Options getDefaultOptions() const;

    const std::string& getFileName() const;

    bool supportsIterator (StageIteratorType t) const
    {   
        if (t == StageIterator_Sequential ) return true;
        if (t == StageIterator_Random ) return true;
        
        return false;
    }

    pdal::StageSequentialIterator* createSequentialIterator() const;
    pdal::StageRandomIterator* createRandomIterator() const;

    // this is called by the stage's iterator
    boost::uint32_t processBuffer(PointBuffer& PointBuffer, std::istream& stream, boost::uint64_t numPointsLeft, LASunzipper* unzipper, ZipPoint* zipPoint) const;

    int getMetadataRecordCount() const;
    const MetadataRecord& getMetadataRecord(int index) const;

    PointFormat getPointFormat() const;
    boost::uint8_t getVersionMajor() const;
    boost::uint8_t getVersionMinor() const;

    boost::uint64_t getPointDataOffset() const;

    // we shouldn't have to expose this
    const std::vector<VariableLengthRecord>& getVLRs() const;

    bool isCompressed() const;

    // for dumping
    virtual boost::property_tree::ptree toPTree() const;

protected:
    const LasHeader& getLasHeader() const { return m_lasHeader; }
    LasHeader& getLasHeaderRef() { return m_lasHeader; }

    MetadataRecord& getMetadataRecordRef(int index);

private:
    std::string m_filename;
    LasHeader m_lasHeader;
    std::vector<MetadataRecord> m_metadataRecords;

    Reader& operator=(const Reader&); // not implemented
    Reader(const Reader&); // not implemented
};


} } } // namespaces

#endif
