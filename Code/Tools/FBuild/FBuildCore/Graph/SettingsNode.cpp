// SettingsNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "SettingsNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( SettingsNode, Node, MetaNone() )
    REFLECT_ARRAY(  m_Environment,              "Environment",              MetaOptional() )
    REFLECT(        m_CachePath,                "CachePath",                MetaOptional() )
    REFLECT(        m_CachePluginDLL,           "CachePluginDLL",           MetaOptional() )
    REFLECT_ARRAY(  m_Workers,                  "Workers",                  MetaOptional() )
    REFLECT(        m_WorkerConnectionLimit,    "WorkerConnectionLimit",    MetaOptional() )
REFLECT_END( SettingsNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
SettingsNode::SettingsNode()
: Node( AString::GetEmpty(), Node::SETTINGS_NODE, Node::FLAG_NONE )
, m_WorkerConnectionLimit( 15 )
{
    // Cache path from environment
    Env::GetEnvVariable( "FASTBUILD_CACHE_PATH", m_CachePathFromEnvVar );
}

// Initialize
//------------------------------------------------------------------------------
bool SettingsNode::Initialize( NodeGraph & /*nodeGraph*/, const BFFIterator & /*iter*/, const Function * /*function*/ )
{
    // using a cache plugin?
    if ( m_CachePluginDLL.IsEmpty() == false )
    {
        FLOG_INFO( "CachePluginDLL: '%s'", m_CachePluginDLL.Get() );
    }

    // "Environment"
    if ( m_Environment.IsEmpty() == false )
    {
        ProcessEnvironment( m_Environment );
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SettingsNode::~SettingsNode() = default;

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool SettingsNode::IsAFile() const
{
    return false;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * SettingsNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    SettingsNode * node = nodeGraph.CreateSettingsNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }
    return node;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void SettingsNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

// GetCachePath
//------------------------------------------------------------------------------
const AString & SettingsNode::GetCachePath() const
{
    // Environment variable takes priority
    if ( m_CachePathFromEnvVar.IsEmpty() == false )
    {
        return m_CachePathFromEnvVar;
    }
    return m_CachePath;
}

// GetCachePluginDLL
//------------------------------------------------------------------------------
const AString & SettingsNode::GetCachePluginDLL() const
{
    return m_CachePluginDLL;
}

// ProcessEnvironment
//------------------------------------------------------------------------------
void SettingsNode::ProcessEnvironment( const Array< AString > & envStrings ) const
{
    // the environment string is used in windows as a double-null terminated string
    // so convert our array to a single buffer

    // work out space required
    uint32_t size = 0;
    for ( uint32_t i=0; i<envStrings.GetSize(); ++i )
    {
        size += envStrings[ i ].GetLength() + 1; // string len inc null
    }

    // allocate space
    AutoPtr< char > envString( (char *)ALLOC( size + 1 ) ); // +1 for extra double-null

    // while iterating, extract the LIB environment variable (if there is one)
    AStackString<> libEnvVar;

    // copy strings end to end
    char * dst = envString.Get();
    for ( uint32_t i=0; i<envStrings.GetSize(); ++i )
    {
        if ( envStrings[ i ].BeginsWith( "LIB=" ) )
        {
            libEnvVar.Assign( envStrings[ i ].Get() + 4, envStrings[ i ].GetEnd() );
        }

        const uint32_t thisStringLen = envStrings[ i ].GetLength();
        AString::Copy( envStrings[ i ].Get(), dst, thisStringLen );
        dst += ( thisStringLen + 1 );
    }

    // final double-null
    *dst = '\000';

    FBuild::Get().SetEnvironmentString( envString.Get(), size, libEnvVar );
}

//------------------------------------------------------------------------------
