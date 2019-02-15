// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ExeNode.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( ExeNode, LinkerNode, MetaNone() )
REFLECT_END( ExeNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
ExeNode::ExeNode()
    : LinkerNode()
{
    m_Type = EXE_NODE;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ExeNode::~ExeNode() = default;

//------------------------------------------------------------------------------
