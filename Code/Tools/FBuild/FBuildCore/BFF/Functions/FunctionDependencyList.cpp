// FunctionAlias
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionDependencyList.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DependencyListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionDependencyList::FunctionDependencyList()
: Function( "DependencyList" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionDependencyList::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionDependencyList::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    // make sure all required variables are defined
    const BFFVariable * sourceV;
    const BFFVariable * dstFileV;
    if ( !GetString( funcStartIter, sourceV, ".Source", true ) ||
         !GetString( funcStartIter, dstFileV, ".Dest", true ) )
    {
        return false; // GetString will have emitted errors
    }

    // Source must not be a path
    if ( PathUtils::IsFolderPath( sourceV->GetString() ) )
    {
        Error::Error_1105_PathNotAllowed( funcStartIter, this, ".Source", sourceV->GetString() );
        return false;
    }

    // Dest must not be a path
    AStackString<> dstFile;
    NodeGraph::CleanPath( dstFileV->GetString(), dstFile );
    if ( PathUtils::IsFolderPath( dstFile ) )
    {
        Error::Error_1105_PathNotAllowed( funcStartIter, this, ".Dest", dstFile );
        return false;
    }

    // Optional pattern
    Array< AString > patterns;
    if ( !GetStrings( funcStartIter, patterns, ".SourcePattern", false ) )
    {
        return false; // GetString will have emitted errors
    }

    // Pre-build dependencies
    Dependencies preBuildDependencies;
    if ( !GetNodeList( nodeGraph, funcStartIter, ".PreBuildDependencies", preBuildDependencies, false ) )
    {
        return false; // GetNodeList will have emitted an error
    }
    Array< AString > preBuildDependencyNames( preBuildDependencies.GetSize(), false );
    for ( const auto & dep : preBuildDependencies )
    {
        preBuildDependencyNames.Append( dep.GetNode()->GetName() );
    }

    // check node doesn't already exist
    if ( nodeGraph.FindNode( dstFile ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, dstFile );
        return false;
    }

    // create our node
    DependencyListNode * depsNode = nodeGraph.CreateDependencyListNode( dstFile );
    depsNode->m_Source = sourceV->GetString();
    depsNode->m_Patterns = patterns;
    depsNode->m_PreBuildDependencyNames = preBuildDependencyNames;
    if ( !depsNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false; // Initialize will have emitted an error
    }

    // handle alias creation
    Dependencies depsNodes;
    depsNodes.Append(Dependency{ depsNode });

    return ProcessAlias( nodeGraph, funcStartIter, depsNodes );
}

//------------------------------------------------------------------------------
