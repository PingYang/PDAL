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

#ifndef INCLUDED_WRITER_HPP
#define INCLUDED_WRITER_HPP

#include <libpc/Filter.hpp>

namespace libpc
{

class LIBPC_DLL Writer
{
public:
    Writer(Stage& prevStage);

    // Implement this in your concrete classes to return a constant string
    // as the name of the stage.  Use upper camel case, with spaces between
    // words.  The last word should generally be "Writer".
    virtual const std::string& getName() const = 0;

    // size of the PointData buffer to use
    void setChunkSize(boost::uint32_t);
    boost::uint32_t getChunkSize() const;

    // Read the  given number of points (or less, if the reader runs out first), 
    // and then write them out to wherever.  Returns total number of points
    // actually written.
    boost::uint64_t write(std::size_t targetNumPointsToWrite);

protected:
    // this is called once before the loop with the writeBuffer calls
    virtual void writeBegin() = 0;

    // called repeatedly, until out of data
    virtual boost::uint32_t writeBuffer(const PointData&) = 0;

    // called once, after the writeBuffer calls
    virtual void writeEnd() = 0;

    Stage& getPrevStage();

    // these two are valid for use after writeBegin has been called
    std::size_t m_actualNumPointsWritten;
    std::size_t m_targetNumPointsToWrite;

private:
    Stage& m_prevStage;
    boost::uint32_t m_chunkSize;
    static const boost::uint32_t s_defaultChunkSize;

    Writer& operator=(const Writer&); // not implemented
    Writer(const Writer&); // not implemented
};

} // namespace libpc

#endif