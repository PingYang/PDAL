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

#include <pdal/PointBuffer.hpp>

#include <boost/lexical_cast.hpp>


namespace pdal
{


PointBuffer::PointBuffer(const SchemaLayout& schemaLayout, boost::uint32_t capacity)
    : m_schemaLayout(schemaLayout)
    , m_data(new boost::uint8_t[m_schemaLayout.getByteSize() * capacity])
    , m_pointSize(m_schemaLayout.getByteSize())
    , m_numPoints(0)
    , m_capacity(capacity)
    , m_bounds(Bounds<double>::getDefaultSpatialExtent())
{

    return;
}

PointBuffer::PointBuffer(PointBuffer const& other) 
    : m_schemaLayout(other.getSchemaLayout())
    , m_data(new boost::uint8_t[m_schemaLayout.getByteSize() * other.m_capacity])
    , m_pointSize(m_schemaLayout.getByteSize())
    , m_numPoints(other.m_numPoints)
    , m_capacity(other.m_capacity)
    , m_bounds(other.m_bounds)
{
    if (other.m_data)
    {
        memcpy(m_data.get(), other.m_data.get(), m_pointSize*m_capacity);
    }

}

PointBuffer& PointBuffer::operator=(PointBuffer const& rhs)

{
    if (&rhs != this)
    {
        m_schemaLayout = rhs.getSchemaLayout();
        m_pointSize = m_schemaLayout.getByteSize();
        m_numPoints = rhs.getNumPoints();
        m_capacity = rhs.getCapacity();
        m_bounds = rhs.getSpatialBounds();
        boost::scoped_array<boost::uint8_t> data( new boost::uint8_t[ m_pointSize*m_capacity ] );
        m_data.swap(data);
        
        if (rhs.m_data.get())
            memcpy(m_data.get(), rhs.m_data.get(), m_pointSize*m_capacity);
        
        
    }
    return *this;
}

PointBuffer::~PointBuffer()
{
}


const Bounds<double>& PointBuffer::getSpatialBounds() const
{
    return m_bounds;
}


void PointBuffer::setSpatialBounds(const Bounds<double>& bounds)
{
    m_bounds = bounds;
}


void PointBuffer::setData(boost::uint8_t* data, std::size_t index)
{
    memcpy(m_data.get() + m_pointSize * index, data, getSchemaLayout().getByteSize());
}

void PointBuffer::setAllData(boost::uint8_t* data, boost::uint32_t byteCount)
{
    memcpy(m_data.get(), data, byteCount);
}

boost::uint32_t PointBuffer::getNumPoints() const
{
    return m_numPoints;
}


void PointBuffer::getData(boost::uint8_t** data, std::size_t* array_size) const
{
    *array_size = getSchemaLayout().getByteSize();
    *data = (boost::uint8_t*) malloc (*array_size);
    memcpy(*data, m_data.get(), *array_size);
}


boost::property_tree::ptree PointBuffer::toPTree() const
{
    boost::property_tree::ptree tree;

    const SchemaLayout& schemaLayout = getSchemaLayout();
    const std::vector<DimensionLayout>& dimensionLayouts = schemaLayout.getDimensionLayouts();
    const boost::uint32_t numPoints = getNumPoints();

    for (boost::uint32_t pointIndex=0; pointIndex<numPoints; pointIndex++)
    {
        const std::string pointstring = boost::lexical_cast<std::string>(pointIndex) + ".";
        
        for (SchemaLayout::DimensionLayoutsCIter citer=dimensionLayouts.begin(); citer != dimensionLayouts.end(); ++citer)
        {
            const DimensionLayout& dimensionLayout = *citer;
            const Dimension& dimension = dimensionLayout.getDimension();
            const std::size_t fieldIndex = dimensionLayout.getPosition();

            const std::string key = pointstring + dimension.getFieldName();
            
            std::string value = "";
            
            switch (dimension.getDataType())
            {
#define GETFIELDAS(T) getField<T>(pointIndex, fieldIndex)
#define STRINGIFY(x) boost::lexical_cast<std::string>(x)
                // note we convert 8-bit fields to ints, so they aren't treated as chars
            case Dimension::Int8:
                value += STRINGIFY((int)GETFIELDAS(boost::int8_t));
                break;
            case Dimension::Uint8:
                value += STRINGIFY((int)GETFIELDAS(boost::uint8_t));
                break;
            case Dimension::Int16:
                value += STRINGIFY(GETFIELDAS(boost::int16_t));
                break;
            case Dimension::Uint16:
                value += STRINGIFY(GETFIELDAS(boost::uint16_t));
                break;
            case Dimension::Int32:
                value += STRINGIFY(GETFIELDAS(boost::int32_t));
                break;
            case Dimension::Uint32:
                value += STRINGIFY(GETFIELDAS(boost::uint32_t));
                break;
            case Dimension::Int64:
                value += STRINGIFY(GETFIELDAS(boost::int64_t));
                break;
            case Dimension::Uint64:
                value += STRINGIFY(GETFIELDAS(boost::uint64_t));
                break;
            case Dimension::Float:
                value += STRINGIFY(GETFIELDAS(float));
                break;
            case Dimension::Double:
                value += STRINGIFY(GETFIELDAS(double));
                break;
            
            default:
                throw pdal_error("unknown dimension data type");
            }

            tree.add(key, value);
        }
    }
    return tree;
}


std::ostream& operator<<(std::ostream& ostr, const PointBuffer& PointBuffer)
{
    using std::endl;

    const SchemaLayout& schemaLayout = PointBuffer.getSchemaLayout();
    const std::vector<DimensionLayout>& dimensionLayouts = schemaLayout.getDimensionLayouts();
    const std::size_t numPoints = PointBuffer.getNumPoints();

    ostr << "Contains " << numPoints << "  points" << endl;

    for (boost::uint32_t pointIndex=0; pointIndex<numPoints; pointIndex++)
    {
       
        ostr << "Point: " << pointIndex << endl;

        for (SchemaLayout::DimensionLayoutsCIter citer=dimensionLayouts.begin(); citer != dimensionLayouts.end(); ++citer)
        {
            const DimensionLayout& dimensionLayout = *citer;
            const Dimension& dimension = dimensionLayout.getDimension();
            std::size_t fieldIndex = dimensionLayout.getPosition();

            ostr << dimension.getFieldName() << " (" << dimension.getDataTypeName(dimension.getDataType()) << ") : ";

            switch (dimension.getDataType())
            {
            case Dimension::Int8:
                ostr << (int)(PointBuffer.getField<boost::int8_t>(pointIndex, fieldIndex));
                break;
            case Dimension::Uint8:
                ostr << (int)(PointBuffer.getField<boost::uint8_t>(pointIndex, fieldIndex));
                break;
            case Dimension::Int16:
                ostr << PointBuffer.getField<boost::int16_t>(pointIndex, fieldIndex);
                break;
            case Dimension::Uint16:
                ostr << PointBuffer.getField<boost::uint16_t>(pointIndex, fieldIndex);
                break;
            case Dimension::Int32:
                ostr << PointBuffer.getField<boost::int32_t>(pointIndex, fieldIndex);
                break;
            case Dimension::Uint32:
                ostr << PointBuffer.getField<boost::uint32_t>(pointIndex, fieldIndex);
                break;
            case Dimension::Int64:
                ostr << PointBuffer.getField<boost::int64_t>(pointIndex, fieldIndex);
                break;
            case Dimension::Uint64:
                ostr << PointBuffer.getField<boost::uint64_t>(pointIndex, fieldIndex);
                break;
            case Dimension::Float:
                ostr << PointBuffer.getField<float>(pointIndex, fieldIndex);
                break;
            case Dimension::Double:
                ostr << PointBuffer.getField<double>(pointIndex, fieldIndex);
                break;
            default:
                throw;
            }

            ostr << endl;
        }
    }

    return ostr;
}


} // namespace pdal
