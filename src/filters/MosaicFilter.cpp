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

#include <pdal/filters/MosaicFilter.hpp>

#include <pdal/Bounds.hpp>
#include <pdal/filters/MosaicFilterIterator.hpp>

namespace pdal { namespace filters {



MosaicFilter::MosaicFilter(const std::vector<Stage*>& prevStages, const Options& options)
    : MultiFilter(prevStages, options)
{
    return;
}


void MosaicFilter::initialize()
{
    MultiFilter::initialize();

    const std::vector<Stage*>& stages = getPrevStages();

    const Stage& stage0 = *stages[0];
    const SpatialReference& srs0 = stage0.getSpatialReference();
    const Schema& schema0 = stage0.getSchema();
    PointCountType pointCountType0 = stage0.getPointCountType();
    boost::uint64_t totalPoints = stage0.getNumPoints();
    Bounds<double> bigbox(stage0.getBounds());

    // we will only mosaic if all the stages have the same core properties: SRS, schema, etc
    for (boost::uint32_t i=1; i<stages.size(); i++)
    {
        Stage& stage = *(stages[i]);
        if (stage.getSpatialReference() != srs0)
            throw impedance_invalid("mosaicked stages must have same srs");
        if (stage.getSchema() != schema0)
            throw impedance_invalid("mosaicked stages must have same schema");
        if (stage.getPointCountType() == PointCount_Unknown)
            pointCountType0 = PointCount_Unknown;
        
        totalPoints += stage.getNumPoints();

        bigbox.grow(stage.getBounds());
    }

    if (pointCountType0 == PointCount_Unknown)
        totalPoints = 0;
    
    setCoreProperties(stage0);
    setPointCountType(pointCountType0);
    setNumPoints(totalPoints);

    return;
}


const Options MosaicFilter::getDefaultOptions() const
{
    Options options;
    return options;
}


pdal::StageSequentialIterator* MosaicFilter::createSequentialIterator() const
{
    return new MosaicFilterSequentialIterator(*this);
}

} } // namespaces
