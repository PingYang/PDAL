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


#include <pdal/drivers/oci/common.hpp>

#include <iostream>

#include <boost/concept_check.hpp> // ignore_unused_variable_warning
#include <boost/make_shared.hpp>

#include <pdal/Bounds.hpp>

namespace pdal { namespace drivers { namespace oci {


OptionsOld GetDefaultOptions() 
{
    OptionsOld options;
    boost::property_tree::ptree& tree = options.GetPTree();
    tree.put("is3d", false);
    tree.put("solid", false);
    tree.put("overwrite", false);
    tree.put("debug", false);
    tree.put("verbose", false);
    tree.put("srid", 4269);
    tree.put("capacity", 8000);
    tree.put("precision", 8);
    tree.put("cloud_id", -1);
    tree.put("connection", std::string(""));
    tree.put("block_table_name", std::string("output"));
    tree.put("block_table_partition_column", std::string(""));
    tree.put("block_table_partition_value", boost::int32_t(0));
    tree.put("base_table_name", std::string("hobu"));
    tree.put("cloud_column_name", std::string("cloud"));
    tree.put("header_blob_column_name", std::string(""));
    tree.put("base_table_aux_columns", std::string(""));
    tree.put("base_table_aux_values", std::string(""));
    tree.put("base_table_boundary_column", std::string(""));
    tree.put("base_table_boundary_wkt", std::string(""));
    tree.put("pre_block_sql", std::string(""));
    tree.put("pre_sql", std::string(""));
    tree.put("post_block_sql", std::string(""));
    tree.put("select_sql", std::string(""));
    tree.put("base_table_bounds", pdal::Bounds<double>());
    tree.put("blob_read_byte_size", boost::uint32_t(2000));
    tree.put("point_schema_override", "");
    
    boost::property_tree::ptree scales;
    scales.put("x", double(0.01));
    scales.put("y", double(0.01));
    scales.put("z", double(0.01));

    boost::property_tree::ptree offsets;
    offsets.put("x", double(0.0));
    offsets.put("y", double(0.0));
    offsets.put("z", double(0.0));
    
    tree.add_child("scale", scales);
    tree.add_child("offset", offsets);
    return options;
}



Block::Block(Connection connection)
    : num_points(0)
    , chunk(new std::vector<boost::uint8_t>)
    , m_connection(connection)
{
    m_connection->CreateType(&blk_extent);
    m_connection->CreateType(&blk_extent->sdo_ordinates, m_connection->GetOrdinateType());
    m_connection->CreateType(&blk_extent->sdo_elem_info, m_connection->GetElemInfoType());
    m_connection->CreateType(&blk_domain);
}

Block::~Block()
{
    m_connection->DestroyType(&blk_domain);
    m_connection->DestroyType(&blk_extent->sdo_elem_info);
    m_connection->DestroyType(&blk_extent->sdo_ordinates);
    
    // FIXME: For some reason having the dtor destroy this
    // causes a segfault
    // m_connection->DestroyType(&blk_extent);
}

Cloud::Cloud(Connection con)
    : connection(con)
{


}

Cloud::~Cloud()
{

}


std::string to_upper(const std::string& input)
{
    std::string inp = std::string(input);
    std::string output = std::string(input);
    
    std::transform(inp.begin(), inp.end(), output.begin(), static_cast < int(*)(int) > (toupper));
    
    return output;
}


pdal::drivers::oci::Connection Connect(Options const& options, bool debug, int verbosity)
{
    std::string connection  = options.getValueOrThrow<std::string>("connection");

    if (connection.empty())
        throw pdal_error("Oracle connection string empty! Unable to connect");

    std::string::size_type slash_pos = connection.find("/",0);
    std::string username = connection.substr(0,slash_pos);
    std::string::size_type at_pos = connection.find("@",slash_pos);

    std::string password = connection.substr(slash_pos+1, at_pos-slash_pos-1);
    std::string instance = connection.substr(at_pos+1);
    
    Connection con = boost::make_shared<OWConnection>(username.c_str(),password.c_str(),instance.c_str());
    
    if (con->Succeeded())
    {
        if (verbosity > 0)
            std::cout << "Oracle connection succeeded" << std::endl;        
    }
    else
        throw connection_failed("Oracle connection failed");
        
    return con;
    
}



}}} // namespace pdal::driver::oci

void CPL_STDCALL OCIGDALErrorHandler(CPLErr eErrClass, int err_no, const char *msg)
{
    std::ostringstream oss;
    
    if (eErrClass == CE_Failure || eErrClass == CE_Fatal) {
        oss <<"GDAL Failure number=" << err_no << ": " << msg;
        throw pdal::pdal_error(oss.str());
    } else {
        return;
    }
}

void CPL_STDCALL OCIGDALDebugErrorHandler(CPLErr eErrClass, int err_no, const char *msg)
{
    std::ostringstream oss;
    
    if (eErrClass == CE_Failure || eErrClass == CE_Fatal) {
        oss <<"GDAL Failure number=" << err_no << ": " << msg;
        throw pdal::pdal_error(oss.str());
    } else if (eErrClass == CE_Debug) {
        std::cout <<"GDAL Debug: " << msg << std::endl;
    } else {
        return;
    }
}
