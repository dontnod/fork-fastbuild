// DependencyListNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "DependencyListNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/Array.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( DependencyListNode, Node, MetaNone() )
    REFLECT(        m_Source,                   "Source",                   MetaFile() )
    REFLECT(        m_Dest,                     "Dest",                     MetaPath() )
    REFLECT_ARRAY(  m_Patterns,                 "Patterns",                 MetaOptional() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,  "PreBuildDependencies",     MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( DependencyListNode )

// Anonymous DependencyListBuilder struct
//------------------------------------------------------------------------------
struct DependencyListBuilder {
    DependencyListBuilder& operator=(DependencyListBuilder&&) = delete; //avoid warning C5027 (move assignment operator was implicitly defined as deleted)
    DependencyListBuilder& operator=(DependencyListBuilder&) = delete; //avoid warning 4626 (assignment operator was implicitly defined as deleted)
    DependencyListBuilder(DependencyListBuilder&) = delete; // avoid warning C4625 (copy constructor was implicitly defined as deleted)

    enum {
        iBuckets = 256,
        mBuckets = (iBuckets - 1)
    };

    using bucket_t = Array< const Node * >;
    bucket_t Visiteds[ iBuckets ];

    const Array< AString > & Patterns;

    explicit DependencyListBuilder( const Array< AString > & patterns )
        : Patterns( patterns )
    {
    }

    bool FindOrAddNode( const Node * node )
    {
        ASSERT( node );

        bucket_t & bucket = Visiteds[ node->GetNameCRC() & mBuckets ];

        if ( bucket.Find( node ) == nullptr )
        {
            bucket.Append( node );
            return true;
        }
        else
        {
            return false;
        }
    }

    void Collect( const Node * node )
    {
        if ( FindOrAddNode( node ) )
        {
            if ( node->GetType() == Node::ALIAS_NODE )
            {
                // resolve aliases to real nodes
                AliasNode * aliasNode = node->CastTo< AliasNode >();
                Collect( aliasNode->GetAliasedNodes() );
            }

            Collect( node->GetStaticDependencies() );
            Collect( node->GetDynamicDependencies() );
        }
    }

    void Collect( const Dependencies & deps )
    {
        for ( const Dependency & dep : deps )
        {
            if ( dep.IsWeak() == false )
            {
                Collect( dep.GetNode() );
            }
        }
    }

    bool IsMatch( const char * filename ) const
    {
        for ( const AString & pattern : Patterns )
        {
            if ( PathUtils::IsWildcardMatch( pattern.Get(), filename ) )
            {
                return true;
            }
        }
        return Patterns.IsEmpty();
    }

    bool DumpTxtFile( const char* filename ) const
    {
        ASSERT( filename );

        // Collect all file dependencies

        Array< const AString * > dependencyList;

        for ( const bucket_t & bucket : Visiteds )
        {
            for ( const Node * node : bucket )
            {
                if ( node->IsAFile() && IsMatch( node->GetName().Get() ) )
                {
                    dependencyList.Append( &node->GetName() );
                }
            }
        }

        // Sort collected filenames

        dependencyList.Sort([]( const AString * a, const AString * b ) {
            return ( a->Compare( *b ) < 0 );
        });

        // Prepare output buffer

        AStackString<> buffer;

        for ( const AString * depname : dependencyList )
        {
            buffer += *depname;
            buffer += "\r\n";
        }

        // Dump to txt file

        FileStream dmp;
        if ( dmp.Open( filename, FileStream::WRITE_ONLY ) == false )
        {
            return false;
        }
        else
        {
            dmp.WriteBuffer( buffer.Get(), buffer.GetLength() * sizeof(*buffer.Get()) );
            dmp.Close();

            return true;
        }
    }

    static bool MakeListTxt( const Node * node, const char * filename, const Array< AString > & patterns )
    {
        DependencyListBuilder builder( patterns );

        // Don't want to collect root node itself

        builder.Collect( node->GetStaticDependencies() );
        builder.Collect( node->GetDynamicDependencies() );

        return builder.DumpTxtFile( filename );
    }
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
DependencyListNode::DependencyListNode()
: FileNode( AString::GetEmpty(), Node::FLAG_NONE )
{
    m_Type = Node::DEPENDENCY_LIST_NODE;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool DependencyListNode::Initialize( NodeGraph & nodeGraph, const BFFToken* iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // Get node for Source of dependency list
    Node* srcNode = nodeGraph.FindNode( m_Source );
    if ( srcNode != nullptr )
    {
        m_StaticDependencies.Append(Dependency( srcNode ));
    }

    return true;
}


// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool DependencyListNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean )
{
    (void)forceClean; // dynamic deps are always re-added here, so this is meaningless

    // clear dynamic deps from previous passes
    m_DynamicDependencies.Clear();

    // Get node for Source of dependency list
    Node* srcNode = nodeGraph.FindNode( m_Source );
    if ( srcNode == nullptr )
    {
        FLOG_ERROR( "failed to find '%s' for dependency list", m_Source.Get() );
        return false;
    }

    m_DynamicDependencies.Append(Dependency( srcNode ));

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
DependencyListNode::~DependencyListNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult DependencyListNode::DoBuild( Job * /*job*/ )
{
    FLOG_OUTPUT("DependencyList: '%s' -> '%s'", m_Source.Get(), GetName().Get());

    // create the dependency list file
    if ( DependencyListBuilder::MakeListTxt( this, m_Name.Get(), m_Patterns ) == false )
    {
        FLOG_ERROR( "DependencyList failed. Error: %s Target: '%s'", LAST_ERROR_STR, GetName().Get() );
        return NODE_RESULT_FAILED; // create file failed
    }

    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

    return NODE_RESULT_OK;
}

//------------------------------------------------------------------------------
