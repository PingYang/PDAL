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

#include <pdal/drivers/liblas/Reader.hpp>

#include <liblas/factory.hpp>
#include <liblas/variablerecord.hpp>

#include <pdal/FileUtils.hpp>
#include <pdal/drivers/liblas/Iterator.hpp>
#include <pdal/drivers/las/VariableLengthRecord.hpp>


namespace pdal { namespace drivers { namespace liblas {

    
Reader::Reader(const Options& options)
    : ReaderBase(options)
    , m_filename(options.getValueOrThrow<std::string>("filename"))
    , m_versionMajor(0)
    , m_versionMinor(0)
    , m_scaleX(0.0)
    , m_scaleY(0.0)
    , m_scaleZ(0.0)
    , m_offsetX(0.0)
    , m_offsetY(0.0)
    , m_offsetZ(0.0)
    , m_isCompressed(false)
    , m_pointFormat(::pdal::drivers::las::PointFormatUnknown)
{
    return;
}


Reader::Reader(const std::string& filename)
    : ReaderBase(Options::none())
    , m_filename(filename)
    , m_versionMajor(0)
    , m_versionMinor(0)
    , m_scaleX(0.0)
    , m_scaleY(0.0)
    , m_scaleZ(0.0)
    , m_offsetX(0.0)
    , m_offsetY(0.0)
    , m_offsetZ(0.0)
    , m_isCompressed(false)
    , m_pointFormat(::pdal::drivers::las::PointFormatUnknown)
{
    return;
}


Reader::~Reader()
{
    return;
}


void Reader::initialize()
{
    pdal::Reader::initialize();

    std::istream* str = FileUtils::openFile(m_filename);

    {
        ::liblas::ReaderFactory factory;
        ::liblas::Reader externalReader = factory.CreateWithStream(*str);

        processExternalHeader(externalReader);

        registerFields(externalReader);
    }

    FileUtils::closeFile(str);

    return;
}


const Options Reader::getDefaultOptions() const
{
    Options options;
    return options;
}


const std::string& Reader::getFileName() const
{
    return m_filename;
}


::pdal::drivers::las::PointFormat Reader::getPointFormat() const
{
    return m_pointFormat;
}


boost::uint8_t Reader::getVersionMajor() const
{
    return m_versionMajor;
}


boost::uint8_t Reader::getVersionMinor() const
{
    return m_versionMinor;
}


const SpatialReference& Reader::getSpatialReference() const
{
    throw not_yet_implemented("SRS support in liblas reader not yet implemented");
}


void Reader::processExternalHeader(::liblas::Reader& externalReader)
{
    const ::liblas::Header& externalHeader = externalReader.GetHeader();

    this->setNumPoints( externalHeader.GetPointRecordsCount() );

    const ::liblas::Bounds<double>& externalBounds = externalHeader.GetExtent();
    const Bounds<double> internalBounds(externalBounds.minx(), externalBounds.miny(), externalBounds.minz(), externalBounds.maxx(), externalBounds.maxy(), externalBounds.maxz());
    this->setBounds(internalBounds);

    m_versionMajor = externalHeader.GetVersionMajor();
    m_versionMinor = externalHeader.GetVersionMinor();

    m_scaleX = externalHeader.GetScaleX();
    m_scaleY = externalHeader.GetScaleY();
    m_scaleZ = externalHeader.GetScaleZ();
    m_offsetX = externalHeader.GetOffsetX();
    m_offsetY = externalHeader.GetOffsetY();
    m_offsetZ = externalHeader.GetOffsetZ();

    m_isCompressed = externalHeader.Compressed();

    m_pointFormat = (::pdal::drivers::las::PointFormat)externalHeader.GetDataFormatId();

    if (::pdal::drivers::las::Support::hasWave(m_pointFormat))
    {
        throw not_yet_implemented("Waveform data (types 4 and 5) not supported");
    }

    return;
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


void Reader::registerFields(::liblas::Reader& externalReader)
{
    const ::liblas::Header& externalHeader = externalReader.GetHeader();
    Schema& schema = getSchemaRef();

    ::pdal::drivers::las::Support::registerFields(schema, getPointFormat());

    ::pdal::drivers::las::Support::setScaling(schema, 
        externalHeader.GetScaleX(),
        externalHeader.GetScaleY(),
        externalHeader.GetScaleZ(),
        externalHeader.GetOffsetX(),
        externalHeader.GetOffsetY(),
        externalHeader.GetOffsetZ());

    return;
}


pdal::StageSequentialIterator* Reader::createSequentialIterator() const
{
    return new SequentialIterator(*this);
}


pdal::StageRandomIterator* Reader::createRandomIterator() const
{
    return new RandomIterator(*this);
}


boost::property_tree::ptree Reader::toPTree() const
{
    boost::property_tree::ptree tree = pdal::Reader::toPTree();

    // add stuff here specific to this stage type

    return tree;
}

} } } // namespaces
