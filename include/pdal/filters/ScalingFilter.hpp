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

#ifndef INCLUDED_FILTERS_SCALINGFILTER_HPP
#define INCLUDED_FILTERS_SCALINGFILTER_HPP

#include <pdal/pdal.hpp>

#include <boost/shared_ptr.hpp>

#include <pdal/Filter.hpp>

namespace pdal
{
    class PointBuffer;
}

namespace pdal { namespace filters {

class ScalingFilterSequentialIterator;

class PDAL_DLL ScalingFilterBase : public Filter
{
protected: // the ctors are protected, since we want people to use the two derived classes below

    // for now...
    //   - we only support scaling of the X,Y,Z fields
    //   - we only support scaling doubles <--> int32s
    // notes:
    //   - "forward=true" means doubles --> ints
    //   - "forward=false" means ints --> doubles
    //   - 1st version uses the scale/offset values already present
    ScalingFilterBase(Stage& prevStage, bool isDescaling, const Options&);
    ScalingFilterBase(Stage& prevStage, bool isDescaling);
    ScalingFilterBase(Stage& prevStage, bool isDescaling, double scaleX, double offsetX, double scaleY, double offsetY, double scaleZ, double offsetZ);

public:
    virtual void initialize();

    bool supportsIterator (StageIteratorType t) const
    {   
        if (t == StageIterator_Sequential ) return true;

        return false;
    }

    pdal::StageSequentialIterator* createSequentialIterator() const;
    pdal::StageRandomIterator* createRandomIterator() const { return NULL; }

    void processBuffer(const PointBuffer& srcData, PointBuffer& dstData) const;

private:
    void checkImpedance();
    
    bool m_customScaleOffset;
    double m_scaleX, m_scaleY, m_scaleZ;
    double m_offsetX, m_offsetY, m_offsetZ;
    bool m_isDescaling;

    ScalingFilterBase& operator=(const ScalingFilterBase&); // not implemented
    ScalingFilterBase(const ScalingFilterBase&); // not implemented
};


class PDAL_DLL ScalingFilter: public ScalingFilterBase
{
public:
    SET_STAGE_NAME("filters.scaling", "Scaling Filter")

    ScalingFilter(Stage& prevStage);
    ScalingFilter(Stage& prevStage, const Options&);
    ScalingFilter(Stage& prevStage, double scaleX, double offsetX, double scaleY, double offsetY, double scaleZ, double offsetZ);

    virtual const Options getDefaultOptions() const;
};


class PDAL_DLL DescalingFilter: public ScalingFilterBase
{
public:
    SET_STAGE_NAME("filters.descaling", "Descaling Filter")

    DescalingFilter(Stage& prevStage);
    DescalingFilter(Stage& prevStage, const Options&);
    DescalingFilter(Stage& prevStage, double scaleX, double offsetX, double scaleY, double offsetY, double scaleZ, double offsetZ);

    virtual const Options getDefaultOptions() const;
};


} } // namespaces

#endif
