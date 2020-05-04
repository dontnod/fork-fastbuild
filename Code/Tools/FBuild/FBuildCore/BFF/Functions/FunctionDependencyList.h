// FunctionDependencyList
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Core
#include "Core/Containers/Array.h"

// FunctionDependencyList
//------------------------------------------------------------------------------
class FunctionDependencyList : public Function
{
public:
    explicit        FunctionDependencyList();
    inline virtual ~FunctionDependencyList() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFToken* funcStartIter ) const override;

};

//------------------------------------------------------------------------------
