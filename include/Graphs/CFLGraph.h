//===----- CFLGraph.h -- Graph for context-free language reachability analysis --//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * CFLGraph.h
 *
 *  Created on: March 5, 2022
 *      Author: Yulei Sui
 */

#ifndef CFLG_H_
#define CFLG_H_

#include <fstream>
#include <iostream>
#include <string>
#include <regex>
#include "CFL/CFLGrammar.h"
#include "Graphs/GenericGraph.h"
#include "Graphs/ConsG.h"

namespace SVF
{
class CFLNode;

typedef GenericEdge<CFLNode> GenericCFLEdgeTy;

class CFLEdge: public GenericCFLEdgeTy
{
public:
    CFLEdge(CFLNode *s, CFLNode *d, GEdgeFlag k = 0):
        GenericCFLEdgeTy(s,d,k)
    {
    }
    ~CFLEdge() override = default;

    inline GEdgeKind getEdgeKind() const
    {
        return this->getEdgeKindWithoutMask();
    }
};


typedef GenericNode<CFLNode,CFLEdge> GenericCFLNodeTy;
class CFLNode: public GenericCFLNodeTy
{
public:
    CFLNode (NodeID i = 0, GNodeK k = 0):
        GenericCFLNodeTy(i, k)
    {
    }
    ~CFLNode() override = default;
};

/// Edge-labeled graph for CFL Reachability analysis
typedef GenericGraph<CFLNode,CFLEdge> GenericCFLGraphTy;
class CFLGraph: public GenericCFLGraphTy
{
public:
    typedef CFLGrammar::Symbol Symbol;
    typedef GenericNode<CFLNode,CFLEdge>::GEdgeSetTy CFLEdgeSet;
    Map<std::string, Symbol> label2SymMap;
    Map<Symbol, std::string> sym2LabelMap;
    Symbol startSymbol;
    Map<CFLGrammar::Kind,  Set<CFLGrammar::Attribute>> kind2AttrMap;
    bool externMap;
    Symbol current;

    CFLGraph()
    {
    }
    ~CFLGraph() override = default;

    virtual void addCFLNode(NodeID id, CFLNode* node);

    virtual const CFLEdge* addCFLEdge(CFLNode* src, CFLNode* dst, CFLEdge::GEdgeFlag label);

    virtual const CFLEdge* hasEdge(CFLNode* src, CFLNode* dst, CFLEdge::GEdgeFlag label);

    void dump(const std::string& filename);

    void view();

    void setMap(Map<std::string, Symbol> &labelMap);
    /// Set label2Sym from External
    void setMap(GrammarBase *grammar);

    /// add attribute to kind2Attribute Map 
    void addAttribute(CFLGrammar::Kind kind, CFLGrammar::Attribute attribute)
    {
        if(kind2AttrMap.find(kind) == kind2AttrMap.end())
        {
            Set<CFLGrammar::Attribute> attrs {attribute};
            kind2AttrMap.insert(make_pair(kind, attrs));
        }
        else
        {
            if(kind2AttrMap[kind].find(attribute) == kind2AttrMap[kind].end())
            {
                kind2AttrMap[kind].insert(attribute);
            }
        }
    }

private:
    CFLEdgeSet cflEdgeSet;
    
};

}

namespace llvm
{
/* !
 * GraphTraits specializations for generic graph algorithms.
 * Provide graph traits for traversing from a constraint node using standard graph traversals.
 */
template<> struct GraphTraits<SVF::CFLNode*> : public GraphTraits<SVF::GenericNode<SVF::CFLNode,SVF::CFLEdge>*  >
{
};

/// Inverse GraphTraits specializations for call graph node, it is used for inverse traversal.
template<>
struct GraphTraits<Inverse<SVF::CFLNode *> > : public GraphTraits<Inverse<SVF::GenericNode<SVF::CFLNode,SVF::CFLEdge>* > >
{
};

template<> struct GraphTraits<SVF::CFLGraph*> : public GraphTraits<SVF::GenericGraph<SVF::CFLNode,SVF::CFLEdge>* >
{
    typedef SVF::CFLNode *NodeRef;
};

} // End namespace llvm

#endif /* CFLG_H_ */