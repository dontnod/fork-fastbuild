// AliasNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "AliasNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileStream.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( AliasNode, Node, MetaNone() )
    REFLECT_ARRAY( m_Targets,   "Targets",          MetaFile() + MetaAllowNonFile() )
    REFLECT( m_Hidden,          "Hidden",           MetaOptional() )
REFLECT_END( AliasNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
AliasNode::AliasNode()
: Node( AString::GetEmpty(), Node::ALIAS_NODE, Node::FLAG_TRIVIAL_BUILD | Node::FLAG_ALWAYS_BUILD )
{
    m_LastBuildTimeMs = 1; // almost no work is done for this node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool AliasNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    Dependencies targets( 32, true );
    const bool allowCopyDirNodes = true;
    const bool allowUnityNodes = true;
    const bool allowRemoveDirNodes = true;
    const bool allowCompilerNodes = true;
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".Targets", m_Targets, targets, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes, allowCompilerNodes ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    m_StaticDependencies = targets;

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
AliasNode::~AliasNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult AliasNode::DoBuild( Job * UNUSED( job ) )
{
	Array< uint64_t > stamps(m_StaticDependencies.GetSize(), false);

    const Dependencies::Iter end = m_StaticDependencies.End();
    for ( Dependencies::Iter it = m_StaticDependencies.Begin();
          it != end;
          ++it )
    {
        // If any nodes are file nodes ...
        const Node * n = it->GetNode();
        if ( n->GetType() == Node::FILE_NODE )
        {
            // ... and the file is missing ...
            if ( n->GetStamp() == 0 )
            {
                // ... the build should fail
                FLOG_ERROR( "Alias: %s\nFailed due to missing file: %s\n", GetName().Get(), n->GetName().Get() );
                return Node::NODE_RESULT_FAILED;
            }
        }
		else if ( n->GetStamp() == 0 )
		{
			// ... the build should fail
			FLOG_ERROR("Alias: %s\nFailed due to missing dependency: %s\n", GetName().Get(), n->GetName().Get());
			return Node::NODE_RESULT_FAILED;
		}

		stamps.Append( n->GetStamp() );
    }

	if ( m_StaticDependencies.IsEmpty() )
	{
		m_Stamp = 1; // Non-zero
	}
	else
	{
		m_Stamp = xxHash::Calc64(&stamps[0], (stamps.GetSize() * sizeof(uint64_t)));
	}

    return NODE_RESULT_OK;
}

//------------------------------------------------------------------------------
