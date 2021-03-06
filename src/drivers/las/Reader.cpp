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

#include <pdal/drivers/las/Reader.hpp>

#ifdef PDAL_HAVE_LASZIP
#include <laszip/lasunzipper.hpp>
#endif

#include <pdal/FileUtils.hpp>
#include <pdal/drivers/las/Header.hpp>
#include <pdal/drivers/las/Iterator.hpp>
#include <pdal/drivers/las/VariableLengthRecord.hpp>
#include "LasHeaderReader.hpp"
#include <pdal/PointBuffer.hpp>
#include "ZipPoint.hpp"

namespace pdal { namespace drivers { namespace las {


Reader::Reader(const Options& options)
    : ReaderBase(options)
    , m_filename(options.getValueOrThrow<std::string>("filename"))
{
    return;
}


Reader::Reader(const std::string& filename)
    : ReaderBase(Options::none())
    , m_filename(filename)
{
    return;
}


void Reader::initialize()
{
    pdal::Reader::initialize();

    std::istream* stream = FileUtils::openFile(m_filename);

    LasHeaderReader lasHeaderReader(m_lasHeader, *stream);
    lasHeaderReader.read( getSchemaRef() );

    this->setBounds(m_lasHeader.getBounds());
    this->setNumPoints(m_lasHeader.GetPointRecordsCount());
    
    // If the user is already overriding this by setting it on the stage, we'll 
    // take their overridden value
    if (getSpatialReference().getWKT(pdal::SpatialReference::eCompoundOK).empty())
    {
        SpatialReference srs;
        m_lasHeader.getVLRs().constructSRS(srs);
        setSpatialReference(srs);
    }

    FileUtils::closeFile(stream);

    return;
}


const Options Reader::getDefaultOptions() const
{
    Option option1("filename", "", "file to read from");
    Options options(option1);
    return options;
}


const std::string& Reader::getFileName() const
{
    return m_filename;
}


int Reader::getMetadataRecordCount() const
{
    return m_metadataRecords.size();
}


const MetadataRecord& Reader::getMetadataRecord(int index) const
{
    return m_metadataRecords[index];
}


MetadataRecord& Reader::getMetadataRecordRef(int index)
{
    return m_metadataRecords[index];
}


PointFormat Reader::getPointFormat() const
{
    return m_lasHeader.getPointFormat();
}


boost::uint8_t Reader::getVersionMajor() const
{
    return m_lasHeader.GetVersionMajor();
}


boost::uint8_t Reader::getVersionMinor() const
{
    return m_lasHeader.GetVersionMinor();
}


const std::vector<VariableLengthRecord>& Reader::getVLRs() const
{
    return m_lasHeader.getVLRs().getAll();
}


boost::uint64_t Reader::getPointDataOffset() const
{
    return m_lasHeader.GetDataOffset();
}


bool Reader::isCompressed() const
{
    return m_lasHeader.Compressed();
}


pdal::StageSequentialIterator* Reader::createSequentialIterator() const
{
    return new SequentialIterator(*this);
}


pdal::StageRandomIterator* Reader::createRandomIterator() const
{
    return new RandomIterator(*this);
}


boost::uint32_t Reader::processBuffer(PointBuffer& data, std::istream& stream, boost::uint64_t numPointsLeft, LASunzipper* unzipper, ZipPoint* zipPoint) const
{
    // we must not read more points than are left in the file
    const boost::uint64_t numPoints64 = std::min<boost::uint64_t>(data.getCapacity(), numPointsLeft);
    const boost::uint32_t numPoints = (boost::uint32_t)std::min<boost::uint64_t>(numPoints64, std::numeric_limits<boost::uint32_t>::max());

    const LasHeader& lasHeader = getLasHeader();
    const SchemaLayout& schemaLayout = data.getSchemaLayout();
    const Schema& schema = schemaLayout.getSchema();
    const PointFormat pointFormat = lasHeader.getPointFormat();

    const PointIndexes indexes(schema, pointFormat);

    const bool hasTime = Support::hasTime(pointFormat);
    const bool hasColor = Support::hasColor(pointFormat);
    const int pointByteCount = Support::getPointDataSize(pointFormat);

    boost::uint8_t* buf = new boost::uint8_t[pointByteCount * numPoints];

    if (zipPoint)
    {
#ifdef PDAL_HAVE_LASZIP
        boost::uint8_t* p = buf;

        bool ok = false;
        for (boost::uint32_t i=0; i<numPoints; i++)
        {
            ok = unzipper->read(zipPoint->m_lz_point);
            if (!ok)
            {
                std::ostringstream oss;
                const char* err = unzipper->get_error();
                if (err==NULL) err="(unknown error)";
                oss << "Error reading compressed point data: " << std::string(err);
                throw pdal_error(oss.str());
            }

            memcpy(p, zipPoint->m_lz_point_data.get(), zipPoint->m_lz_point_size);
            p +=  zipPoint->m_lz_point_size;
        }
#else
                throw pdal_error("LASzip is not enabled for this pdal::drivers::las::Reader::processBuffer");
#endif
    }
    else
    {
        Utils::read_n(buf, stream, pointByteCount * numPoints);
    }

    for (boost::uint32_t pointIndex=0; pointIndex<numPoints; pointIndex++)
    {
        boost::uint8_t* p = buf + pointByteCount * pointIndex;

        // always read the base fields
        {
            const boost::int32_t x = Utils::read_field<boost::int32_t>(p);
            const boost::int32_t y = Utils::read_field<boost::int32_t>(p);
            const boost::int32_t z = Utils::read_field<boost::int32_t>(p);
            const boost::uint16_t intensity = Utils::read_field<boost::uint16_t>(p);
            const boost::uint8_t flags = Utils::read_field<boost::uint8_t>(p);
            const boost::uint8_t classification = Utils::read_field<boost::uint8_t>(p);
            const boost::int8_t scanAngleRank = Utils::read_field<boost::int8_t>(p);
            const boost::uint8_t user = Utils::read_field<boost::uint8_t>(p);
            const boost::uint16_t pointSourceId = Utils::read_field<boost::uint16_t>(p);

            const boost::uint8_t returnNum = flags & 0x07;
            const boost::uint8_t numReturns = (flags >> 3) & 0x07;
            const boost::uint8_t scanDirFlag = (flags >> 6) & 0x01;
            const boost::uint8_t flight = (flags >> 7) & 0x01;

            data.setField<boost::uint32_t>(pointIndex, indexes.X, x);
            data.setField<boost::uint32_t>(pointIndex, indexes.Y, y);
            data.setField<boost::uint32_t>(pointIndex, indexes.Z, z);
            data.setField<boost::uint16_t>(pointIndex, indexes.Intensity, intensity);
            data.setField<boost::uint8_t>(pointIndex, indexes.ReturnNumber, returnNum);
            data.setField<boost::uint8_t>(pointIndex, indexes.NumberOfReturns, numReturns);
            data.setField<boost::uint8_t>(pointIndex, indexes.ScanDirectionFlag, scanDirFlag);
            data.setField<boost::uint8_t>(pointIndex, indexes.EdgeOfFlightLine, flight);
            data.setField<boost::uint8_t>(pointIndex, indexes.Classification, classification);
            data.setField<boost::int8_t>(pointIndex, indexes.ScanAngleRank, scanAngleRank);
            data.setField<boost::uint8_t>(pointIndex, indexes.UserData, user);
            data.setField<boost::uint16_t>(pointIndex, indexes.PointSourceId, pointSourceId);
        }

        if (hasTime)
        {
            const double time = Utils::read_field<double>(p);
            data.setField<double>(pointIndex, indexes.Time, time);
        }

        if (hasColor)
        {
            const boost::uint16_t red = Utils::read_field<boost::uint16_t>(p);
            const boost::uint16_t green = Utils::read_field<boost::uint16_t>(p);
            const boost::uint16_t blue = Utils::read_field<boost::uint16_t>(p);

            data.setField<boost::uint16_t>(pointIndex, indexes.Red, red);
            data.setField<boost::uint16_t>(pointIndex, indexes.Green, green);
            data.setField<boost::uint16_t>(pointIndex, indexes.Blue, blue);       
        }
        
        data.setNumPoints(pointIndex+1);
    }

    delete[] buf;

    data.setSpatialBounds( lasHeader.getBounds() );

    return numPoints;
}


boost::property_tree::ptree Reader::toPTree() const
{
    boost::property_tree::ptree tree = pdal::Reader::toPTree();

    tree.add("Compression", this->isCompressed());
    // add more stuff here specific to this stage type

    return tree;
}

} } } // namespaces
