/******************************************************************************
* Copyright (c) 2011, Howard Butler <hobu.inc@gmail.com>
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

#include <pdal/filters/ByteSwapFilterIterator.hpp>

#include <pdal/filters/ByteSwapFilter.hpp>
#include <pdal/PointBuffer.hpp>
#include <pdal/filters/Chipper.hpp>
#include <iostream>
namespace pdal { namespace filters {


ByteSwapFilterSequentialIterator::ByteSwapFilterSequentialIterator(const ByteSwapFilter& filter)
    : pdal::FilterSequentialIterator(filter)
    , m_swapFilter(filter)
{
    return;
}


boost::uint64_t ByteSwapFilterSequentialIterator::skipImpl(boost::uint64_t count)
{
    return naiveSkipImpl(count);
}


boost::uint32_t ByteSwapFilterSequentialIterator::readBufferImpl(PointBuffer& dstData)
{
    // The client has asked us for dstData.getCapacity() points.
    // We will read from our previous stage until we get that amount (or
    // until the previous stage runs out of points).
    
    Stage const* prevStage = &(m_swapFilter.getPrevStage());

        // std::cout << "Source: " << dstData.getSchemaLayout().getSchema() << std::endl;
        // std::cout << "prev stage: " << prevStage->getSchema() << std::endl;    
    Chipper const* chip = dynamic_cast<Chipper const*>(prevStage);
    
    if (chip)
    {
        PointBuffer srcData(dstData.getSchemaLayout(), dstData.getCapacity());
        const boost::uint32_t numSrcPointsRead = getPrevIterator().read(srcData);
        const boost::uint32_t numPointsProcessed = m_swapFilter.processBuffer(dstData, srcData);
                        
        assert (numSrcPointsRead == numPointsProcessed);
        // std::cout << "Prev stage was a chipper!" << std::endl;
        return numPointsProcessed;
    }

    boost::uint32_t numPointsNeeded = dstData.getCapacity();
    boost::uint32_t numPointsAchieved = 0;
    // assert(dstData.getNumPoints() == 0);
    
    while (numPointsNeeded > 0)
    {
        // set up buffer to be filled by prev stage
        PointBuffer srcData(dstData.getSchemaLayout(), numPointsNeeded);

    
        // read from prev stage
        const boost::uint32_t numSrcPointsRead = getPrevIterator().read(srcData);
        assert(numSrcPointsRead == srcData.getNumPoints());
        assert(numSrcPointsRead <= numPointsNeeded);

        numPointsAchieved += numSrcPointsRead;
            
        // we got no data, and there is no more to get -- exit the loop
        if (numSrcPointsRead == 0) return numPointsAchieved;
    
        // copy points from src (prev stage) into dst (our stage), 
        // based on the CropFilter's rules (i.e. its bounds)
        const boost::uint32_t numPointsProcessed = m_swapFilter.processBuffer(dstData, srcData);
    
        numPointsNeeded -= numPointsProcessed;

    }
    



    return numPointsAchieved;
}


bool ByteSwapFilterSequentialIterator::atEndImpl() const
{
    // we don't have a fixed point point --
    // we are at the end only when our source is at the end
    const StageSequentialIterator& iter = getPrevIterator();
    return iter.atEnd();
}


} } // namespaces
