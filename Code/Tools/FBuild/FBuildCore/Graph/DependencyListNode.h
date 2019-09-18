// DependencyListNode.h - a node that list dependencies of a target to a file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// DependencyListNode
//------------------------------------------------------------------------------
class DependencyListNode : public FileNode
{
    REFLECT_NODE_DECLARE( DependencyListNode )
public:
    explicit DependencyListNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function ) override;
    virtual ~DependencyListNode() override;

    static inline Node::Type GetTypeS() { return Node::COPY_FILE_NODE; }

private:
    virtual BuildResult DoBuild( Job * job ) override;

    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;

    friend class FunctionDependencyList;
    AString             m_Source;
    AString             m_Dest;
    Array< AString >    m_Patterns;
    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
