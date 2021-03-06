/******************************************************************************
* Copyright (c) 2011, Howard Butler, hobu.inc@gmail.com
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

#include <pdal/drivers/oci/Iterator.hpp>

#include <pdal/PointBuffer.hpp>
#include <pdal/drivers/oci/Reader.hpp>
#include <pdal/Vector.hpp>

#include <sstream>
#include <map>
#include <algorithm>


namespace pdal { namespace drivers { namespace oci {

IteratorBase::IteratorBase(const Reader& reader)
    : m_statement(Statement())
    , m_at_end(false)
    , m_cloud(reader.getCloud())
    , m_reader(reader)

{
    
    std::ostringstream select_blocks;
    
    select_blocks
        << "select T.OBJ_ID, T.BLK_ID, T.BLK_EXTENT, T.NUM_POINTS, T.POINTS from " 
        << m_cloud->blk_table << " T WHERE T.OBJ_ID = " 
        << m_cloud->pc_id;


    m_statement = Statement(m_cloud->connection->CreateStatement(select_blocks.str().c_str()));

    m_statement->Execute(0);
    m_block = defineBlock(m_statement);
    
    return;
}


IteratorBase::~IteratorBase()
{
}



const Reader& IteratorBase::getReader() const
{
    return m_reader;
}


boost::uint32_t IteratorBase::myReadBuffer(PointBuffer& data)
{
    boost::uint32_t numPointsRead = 0;

    data.setNumPoints(0);
    
    bool bDidRead = false;


    // std::cout << "m_block->num_points: " << m_block->num_points << std::endl;
    if (!m_block->num_points) 
    {
        // We still have a block of data from the last readBuffer call
        // that was partially read. 
        // std::cout << "reading because we have no points" << std::endl;
        bDidRead = m_statement->Fetch();        
        if (!bDidRead)
        {
            m_at_end = true;
            return 0;
        }
        
        if (m_block->num_points > static_cast<boost::int32_t>(data.getCapacity()))
        {
            throw buffer_too_small("The PointBuffer is too small to contain this block.");
        }
    
    } else 
    {
        // Our read was already "done" last readBuffer call, but if we're done,
        // we're done
        if (m_at_end) return 0;
        bDidRead = true;

    }
    
    while (bDidRead)
    {
        boost::uint32_t numReadThisBlock = m_block->num_points;
        if (numReadThisBlock > (data.getCapacity() - data.getNumPoints()))
        {
            // We're done.  We still have more data, but the 
            // user is going to have to request another buffer.
            // We're not going to fill the buffer up to *exactly* 
            // the number of points the user requested.  
            // If the buffer's capacity isn't large enough to hold 
            // an oracle block, they're just not going to get anything 
            // back right now (FIXME)
            break;
        }

        numPointsRead = numPointsRead + numReadThisBlock;
        boost::uint32_t nAmountRead = 0;
    
    
        boost::uint32_t blob_length = m_statement->GetBlobLength(m_block->locator);
    
        if (m_block->chunk->size() < blob_length)
        {
            m_block->chunk->resize(blob_length);
        }
        
        // std::cout << "blob_length: " << blob_length << std::endl;

        bool read_all_data = m_statement->ReadBlob( m_block->locator,
                                         (void*)(&(*m_block->chunk)[0]),
                                         m_block->chunk->size() , 
                                         &nAmountRead);
        if (!read_all_data) throw pdal_error("Did not read all blob data!");

        // std::cout << "nAmountRead: " << nAmountRead << std::endl;
        
        data.setNumPoints(numPointsRead);
        data.setAllData(&(*m_block->chunk)[0], m_block->chunk->size());
        // unpackOracleData(data);
        bDidRead = m_statement->Fetch();
        if (!bDidRead)
        {
            m_at_end = true;
            return numPointsRead;
        }
    }    
    



    double x, y, z;
    // 
    // boost::int32_t elem1, elem2, elem3;
    // m_statement->GetElement(&(m_block->blk_extent->sdo_elem_info), 0, &elem1);
    // m_statement->GetElement(&(m_block->blk_extent->sdo_elem_info), 1, &elem2);
    // m_statement->GetElement(&(m_block->blk_extent->sdo_elem_info), 2, &elem3);
    // 
    // boost::int32_t gtype, srid;
    // gtype= m_statement->GetInteger(&(m_block->blk_extent->sdo_gtype));
    // srid =m_statement->GetInteger(&(m_block->blk_extent->sdo_srid));

    
    // See http://download.oracle.com/docs/cd/B28359_01/appdev.111/b28400/sdo_objrelschema.htm#g1013735

    // boost::uint32_t dimension = gtype / 1000;
    // boost::uint32_t geom_type = gtype % 100;
    // boost::uint32_t referencing= ((gtype % 1000) / 100);
    // std::cout << "dimension: " << dimension << " geometry type: " << geom_type << " referencing " << referencing << std::endl;
    
    pdal::Vector<double> mins;
    pdal::Vector<double> maxs;
    
    boost::int32_t bounds_length = m_statement->GetArrayLength(&(m_block->blk_extent->sdo_ordinates));
    
    for (boost::int32_t i = 0; i < bounds_length; i = i + 2)
    {
        double v;
        m_statement->GetElement(&(m_block->blk_extent->sdo_ordinates), i, &v);
        mins.add(v);
        m_statement->GetElement(&(m_block->blk_extent->sdo_ordinates), i+1, &v);
        maxs.add(v);
    }
    
    pdal::Bounds<double> block_bounds(mins, maxs);
    
    data.setSpatialBounds(block_bounds);
    
    // std::cout << "srid: " << srid << std::endl;
    
    // std::cout << "elem1, elem2, elem3 " << elem1 << " " << elem2 << " " << elem3 << std::endl;
    
    // std::cout << "elem_info size " << m_statement->GetArrayLength(&(m_block->blk_extent->sdo_elem_info)) << std::endl;
    // std::cout << "sdo_ordinates size " << m_statement->GetArrayLength(&(m_block->blk_extent->sdo_ordinates)) << std::endl;

    m_statement->GetElement(&(m_block->blk_extent->sdo_ordinates), 0, &x);
    m_statement->GetElement(&(m_block->blk_extent->sdo_ordinates), 1, &y);
    m_statement->GetElement(&(m_block->blk_extent->sdo_ordinates), 2, &z);

    // std::cout << "x, y, z " << x << " " << y << " " << z << std::endl;


    return numPointsRead;
}





//---------------------------------------------------------------------------
//
// SequentialIterator
//
//---------------------------------------------------------------------------

SequentialIterator::SequentialIterator(const Reader& reader)
    : IteratorBase(reader)
    , pdal::StageSequentialIterator(reader)
{
    return;
}


SequentialIterator::~SequentialIterator()
{
    return;
}


boost::uint64_t SequentialIterator::skipImpl(boost::uint64_t count)
{
    // const boost::uint64_t newPos64 = getIndex() + count;
    // 
    // // The liblas reader's seek() call only supports size_t, so we might
    // // not be able to satisfy this request...
    // 
    // if (newPos64 > std::numeric_limits<size_t>::max())
    // {
    //     throw pdal_error("cannot support seek offsets greater than 32-bits");
    // }
    // 
    // // safe cast, since we just handled the overflow case
    // size_t newPos = static_cast<size_t>(newPos64);
    // 
    // getExternalReader().Seek(newPos);

    return 0;
}


BlockPtr IteratorBase::defineBlock(Statement statement)
{

    int   iCol = 0;
    char  szFieldName[OWNAME];
    int   hType = 0;
    int   nSize = 0;
    int   nPrecision = 0;
    signed short nScale = 0;
    char szTypeName[OWNAME];
    
    BlockPtr block = BlockPtr(new Block(m_cloud->connection));

    m_cloud->connection->CreateType(&(block->blk_extent));    

    while( statement->GetNextField(iCol, szFieldName, &hType, &nSize, &nPrecision, &nScale, szTypeName) )
    {
        std::string name = to_upper(std::string(szFieldName));

        if (Utils::compare_no_case(szFieldName, "OBJ_ID") == 0)
        {
            statement->Define(&(block->obj_id));
        }

        if (Utils::compare_no_case(szFieldName, "BLK_ID") == 0)
        {
            statement->Define(&(block->blk_id));
        }

        if (Utils::compare_no_case(szFieldName, "BLK_EXTENT") == 0)
        {
            statement->Define(&(block->blk_extent));
        }

        if (Utils::compare_no_case(szFieldName, "BLK_DOMAIN") == 0)
        {
            statement->Define(&(block->blk_domain));
        }
        
        if (Utils::compare_no_case(szFieldName, "PCBLK_MIN_RES") == 0)
        {
            statement->Define(&(block->pcblk_min_res));
        }

        if (Utils::compare_no_case(szFieldName, "PCBLK_MAX_RES") == 0)
        {
            statement->Define(&(block->pcblk_max_res));
        }

        if (Utils::compare_no_case(szFieldName, "NUM_POINTS") == 0)
        {
            statement->Define(&(block->num_points));
        }

        if (Utils::compare_no_case(szFieldName, "NUM_UNSORTED_POINTS") == 0)
        {
            statement->Define(&(block->num_unsorted_points));
        }

        if (Utils::compare_no_case(szFieldName, "PT_SORT_DIM") == 0)
        {
            statement->Define(&(block->pt_sort_dim));
        }

        if (Utils::compare_no_case(szFieldName, "POINTS") == 0)
        {
            statement->Define( &(block->locator) ); 
        }
        iCol++;
    }
    
    return block;
}


bool SequentialIterator::atEndImpl() const
{
    return m_at_end; 
    // return getIndex() >= getStage().getNumPoints();
}


boost::uint32_t SequentialIterator::readBufferImpl(PointBuffer& data)
{
    return myReadBuffer(data);
}


//---------------------------------------------------------------------------
//
// RandomIterator
//
//---------------------------------------------------------------------------


} } } // namespaces
