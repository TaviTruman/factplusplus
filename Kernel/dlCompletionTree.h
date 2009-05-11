/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2003-2009 by Dmitry Tsarkov

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _DLCOMPLETIONTREE_H
#define _DLCOMPLETIONTREE_H

#include <vector>
#include <iostream>

#include "globaldef.h"
#include "dlCompletionTreeArc.h"
#include "tSaveList.h"
#include "tRestorer.h"
#include "CGLabel.h"
#include "logging.h"

class DLDag;
class DlCompletionGraph;

// use the following to control logging information about saving/restoring nodes
#define RKG_CHECK_BACKJUMPING

//#include "SmallObj.h"

extern void printBlockingStat ( std::ostream& o );
extern void clearBlockingStat ( void );

/// level of CTree's nominal node
typedef unsigned short CTNominalLevel;
/// default level for the Blockable node
const CTNominalLevel BlockableLevel = static_cast<CTNominalLevel>(-1);

class DlCompletionTree//: public Loki::SmallObject<>
{
	friend class DlCompletionGraph;

protected:	// internal classes
		/// class for saving Completion Tree nodes state
	class SaveState
	{
	public:		// members
			/// saving status of the label
		CGLabel::SaveState lab;
			/// curLevel of the Node structure
		unsigned int curLevel;
			/// amount of parents
		unsigned int nPars;
			/// amount of children
		unsigned int nSons;

	private:	// protection from copying
			/// no assignment
		SaveState& operator = ( const SaveState& node );

	public:		// interface
			/// empty c'tor
		SaveState ( void ) {}
			/// copy c'tor
		SaveState ( const SaveState& node )
			: lab (node.lab)
			, curLevel (node.curLevel)
			, nPars (node.nPars)
			, nSons (node.nSons)
			{}
			/// empty d'tor
		~SaveState ( void ) {}

			/// get level of a saved node
		unsigned int level ( void ) const { return curLevel; }
	}; // SaveState

		/// restore blocked node
	class UnBlock: public TRestorer
	{
	protected:
		DlCompletionTree* p;
		const DlCompletionTree* Blocker;
		DepSet dep;
		bool pBlocked, dBlocked;
	public:
		UnBlock ( DlCompletionTree* q ) : p(q), Blocker(q->Blocker), dep(q->pDep), pBlocked(q->pBlocked), dBlocked(q->dBlocked) {}
		virtual ~UnBlock ( void ) {}
		void restore ( void ) { p->Blocker = Blocker; p->pDep = dep; p->pBlocked = pBlocked; p->dBlocked = dBlocked; }
	}; // UnBlock

		/// restore (un)cached node
	class CacheRestorer: public TRestorer
	{
	protected:
		DlCompletionTree* p;
		bool cached;
	public:
		CacheRestorer ( DlCompletionTree* q ) : p(q), cached(q->cached) {}
		virtual ~CacheRestorer ( void ) {}
		void restore ( void ) { p->cached = cached; }
	}; // CacheRestorer

#ifdef RKG_IR_IN_NODE_LABEL
		/// restore node after IR set change
	class IRRestorer: public TRestorer
	{
	protected:
		DlCompletionTree* p;
		unsigned int n;
	public:
		IRRestorer ( DlCompletionTree* q ) : p(q), n(q->IR.size()) {}
		virtual ~IRRestorer ( void ) {}
		void restore ( void ) { p->IR.reset(n); }
	}; // IRRestorer
#endif

public:		// type interface
		/// type for set of arcs
	typedef std::vector<DlCompletionTreeArc*> ArcCollection;

		/// iterator on edges
	typedef ArcCollection::iterator edge_iterator;
		/// const iterator on edges
	typedef ArcCollection::const_iterator const_edge_iterator;

		/// RO iterator on label
	typedef CGLabel::const_iterator const_label_iterator;

#ifdef RKG_IR_IN_NODE_LABEL
		/// type for inequality relation information
	typedef growingArray<ConceptWDep> IRInfo;
#endif

protected:	// global vars
		/// DAG of operands
	static DLDag* pDLHeap;

public:		// static interface
		/// init static context for DL trees
	static void initContext ( DLDag* pDag ) { pDLHeap = pDag; }

protected:	// members
		/// label of a node
	CGLabel Label;
#ifdef RKG_IR_IN_NODE_LABEL
		/// inequality relation information respecting current node
	IRInfo IR;
#endif
		/// Parents information
	ArcCollection Parent;
		/// Children information
	ArcCollection Son;
		/// pointer to last saved node
	TSaveList<SaveState> saves;
		/// ID of node (used in print)
	unsigned int id;
		/// level of a nominal node; 0 means blockable one
	CTNominalLevel nominalLevel;
		/// concept that init the newly created node
	BipolarPointer Init;

		/// blocker of a node
	const DlCompletionTree* Blocker;
		/// dep-set for Purge op
	DepSet pDep;

	// save state information
	unsigned int curLevel;	// current level

		/// is given node a data node
	unsigned int flagDataNode : 1;
		/// flag if node is Cached
	unsigned int cached : 1;
		/// flag whether node is permanently/temporarily blocked
	unsigned int pBlocked : 1;
		/// flag whether node is directly/indirectly blocked
	unsigned int dBlocked : 1;
		/** Whether node is affected by change of some potential blocker.
			This flag may be viewed as a cache for a 'blocked' status
		*/
	unsigned int affected : 1;
		/// the rest
	unsigned int unused : 27;

private:	// methods
		/// no copy c'tor
	DlCompletionTree ( const DlCompletionTree& copy );
		/// no assignment
	DlCompletionTree& operator = ( const DlCompletionTree& copy );

protected:	// methods

	//----------------------------------------------
	// blocking support methods
	//----------------------------------------------

	// sub-methods for optimal blocking

		/// check B1 and B2 from optimal blocking for given blocker candidate
	bool isCommonlyBlockedBy ( const DlCompletionTree* p ) const;
		/// check B3 and B4 from optimal blocking for given blocker candidate
	bool isABlockedBy ( const DlCompletionTree* p ) const;
		/// check B5 and B6 from optimal blocking for given blocker candidate
	bool isCBlockedBy ( const DlCompletionTree* p ) const;

	// checking the blocking conditions for optimized blocking

		/// check if B1 holds for a given vertex (p is a candidate for blocker)
	bool B1 ( const DlCompletionTree* p ) const;
		/// check if B2 holds for (AS C) a simple automaton A for S
	bool B2 ( const RoleAutomaton& A, BipolarPointer C ) const;
		/// check if B2 holds for C=(AS{n} X) a complex automaton A for S
	bool B2 ( const RoleAutomaton& A, BipolarPointer C, RAState n ) const;
		/// check if B2 holds for given DL vertex with C=V
	bool B2 ( const DLVertex& v, BipolarPointer C ) const
	{
#	ifdef ENABLE_CHECKING
		assert ( hasParent() );	// safety
#	endif

		if ( v.getRole()->isSimple() )
			return B2 ( v.getRole()->getAutomaton(), v.getC() );
		else
			return B2 ( v.getRole()->getAutomaton(), C, v.getState() );
	}
		/// check if B3 holds for (<= n S.C)\in w' (p is a candidate for blocker)
	bool B3 ( const DlCompletionTree* p, unsigned int n, const TRole* S, BipolarPointer C ) const;
		/// check if B4 holds for (>= m T.E)\in w' (p is a candidate for blocker)
	bool B4 ( const DlCompletionTree* p, unsigned int m, const TRole* T, BipolarPointer E ) const;
		/// check if B5 holds for (<= n T.E)\in w'
	bool B5 ( const TRole* T, BipolarPointer E ) const;
		/// check if B6 holds for (>= m U.F)\in v
	bool B6 ( const TRole* U, BipolarPointer F ) const;

	//----------------------------------------------
	// re-building blocking hierarchy
	//----------------------------------------------

		/// check whether a node can block another one with init concept C
	bool canBlockInit ( BipolarPointer C ) const { return C == bpTOP || label().contains(C); }
		/// check if all parent arcs are blocked
	bool isParentArcIBlocked ( void ) const
	{
		for ( const_edge_iterator p = beginp(); p != endp(); ++p )
			if ( !(*p)->isIBlocked() )
				return false;
		return true;
	}

	//----------------------------------------------
	// Transitive SOME support interface
	//----------------------------------------------

		/// check if SOME rule is applicable for transitive R
	const DlCompletionTree* isTSomeApplicable ( const TRole* R, BipolarPointer C ) const;
		/// check if SOME rule is applicable for non-transitive R
	const DlCompletionTree* isNSomeApplicable ( const TRole* R, BipolarPointer C ) const;
		/// check if transitive R-successor labelled with C
	const DlCompletionTree* isTSuccLabelled ( const TRole* R, BipolarPointer C ) const;
		/// check if transitive R-predecessor labelled with C; skip FROM node
	const DlCompletionTree* isTPredLabelled ( const TRole* R, BipolarPointer C, const DlCompletionTree* from ) const;

	//----------------------------------------------
	//	checking edge labelling
	//----------------------------------------------

		/// @return an edge from BEG to END ending in P and labelled by R; @return NULL if no such edge found
	template<class Iterator>
	DlCompletionTreeArc* getEdgeLabelled ( Iterator beg, Iterator end,
										   const TRole* R, const DlCompletionTree* p ) const
	{
		for ( ; beg != end; ++beg )
			if ( (*beg)->getArcEnd() == p && (*beg)->isNeighbour(R) )
				return *beg;
		return NULL;
	}


	//----------------------------------------------
	// inequality relation methods
	//----------------------------------------------

		/// check if the current node is in IR wrt C; if so, write the clash-set to DEP
	bool inIRwithC ( const ConceptWDep& C, DepSet& dep ) const;

	//----------------------------------------------
	// saving/restoring
	//----------------------------------------------

		/// save current state to given SS
	void save ( SaveState* nss ) const;
		/// restore state from given SS; delete SS after
	void restore ( SaveState* nss );

	//----------------------------------------------
	// logging/output
	//----------------------------------------------

		/// log saving/restoring node
	void logSRNode ( const char* action ATTR_UNUSED ) const
	{
#	if defined(RKG_CHECK_BACKJUMPING)
		if ( LLM.isWritable(llSRInfo) )
			LL << " " << action << "(" << id << "[" << Parent.size() << "," << Son.size() << "]," << curLevel << ")";
#		undef RKG_CHECK_BACKJUMPING		// it is the only user
#	endif
	}
		/// get letter corresponding to the blocking mode
	const char* getBlockingStatusName ( void ) const { return isPBlocked() ? "p" : isDBlocked() ? "d" : isIBlocked() ? "i" : "u"; }
		/// log node status (d-,i-,p-blocked or cached
	void logNodeBStatus ( std::ostream& o ) const
	{
		// blocking status information
		if ( Blocker )
			o << getBlockingStatusName() << Blocker->getId();
		if ( isCached() )
			o << "c";
	}
 		/// log if node became p-blocked
	void logNodeBlocked ( void ) const
	{
		if ( LLM.isWritable(llGTA) )
		{
			LL << " " << getBlockingStatusName() << "b(" << id;
			if ( Blocker )
				LL << "," << Blocker->id;
			LL << ")";
		}
	}

public:		// methods
		/// init newly created node with starting LEVEL
	void init ( unsigned int level );
		/// c'tor: create an empty node
	DlCompletionTree ( unsigned int newId ) : id(newId) {}
		/// d'tor: delete node
	~DlCompletionTree ( void ) { saves.clear(); }

		/// add given arc as a parent
	void addParent ( DlCompletionTreeArc* parent ) { Parent.push_back(parent); }
		/// add given arc as a child
	void addChild ( DlCompletionTreeArc* child ) { Son.push_back(child); }

		/// get Node's id
	unsigned int getId ( void ) const { return id; }
		/// check if the node is cached (IE need not to be expanded)
	bool isCached ( void ) const { return cached; }
		/// set cached status of given node
	TRestorer* setCached ( bool val )
	{
		if ( cached == val )
			return NULL;
		TRestorer* ret = new CacheRestorer(this);
		cached = val;
		return ret;
	}

	// data node methods

	bool isDataNode ( void ) const { return flagDataNode; }
	void setDataNode ( void ) { flagDataNode = true; }

	// nominal node methods

	bool isBlockableNode ( void ) const { return nominalLevel == BlockableLevel; }
	bool isNominalNode ( void ) const { return nominalLevel != BlockableLevel; }
	bool isNominalNode ( CTNominalLevel level ) const { return nominalLevel == level; }
	void setNominalLevel ( void ) { nominalLevel = 0; }
	void setNominalLevel ( CTNominalLevel newLevel ) { nominalLevel = newLevel; }
	CTNominalLevel getNominalLevel ( void ) const { return nominalLevel; }

		/// compare 2 CT nodes wrt their nominal level/status
	bool operator < ( const DlCompletionTree& node ) const
	{
		return ( getNominalLevel() < node.getNominalLevel() ) ||
			( ( getNominalLevel() == node.getNominalLevel() ) &&
				( getId() < node.getId() ) );
	}

		/// adds concept P to a label, defined by TAG; update blocked status if necessary
	void addConcept ( const ConceptWDep& p, DagTag tag ) { Label.getLabel(tag).add(p); }
		/// set the Init concept
	void setInit ( BipolarPointer p ) { Init = p; }

	//----------------------------------------------
	// children/parent access interface
	//----------------------------------------------

		/// return true if node is a non-root; works for reflexive roles
	bool hasParent ( void ) const
	{
		if ( Parent.empty() )
			return false;
		// root nodes with reflexive roles will have first upcoming edge to itself;
		// every non-root node will have first upcoming edge pointed to a parent
		return (*beginp())->getArcEnd() != this;
	}
		/// return true if node has any children (including itself in case of reflexive roles)
	bool hasChildren ( void ) const { return !Son.empty(); }

	// parent iterators
	const_edge_iterator beginp ( void ) const { return Parent.begin(); }
	const_edge_iterator endp ( void ) const { return Parent.end(); }
	edge_iterator beginp ( void ) { return Parent.begin(); }
	edge_iterator endp ( void ) { return Parent.end(); }

	// children iterators
	const_edge_iterator begins ( void ) const { return Son.begin(); }
	const_edge_iterator ends ( void ) const { return Son.end (); }
	edge_iterator begins ( void ) { return Son.begin(); }
	edge_iterator ends ( void ) { return Son.end (); }

	//----------------------------------------------
	// Transitive SOME support interface
	//----------------------------------------------

		/// check if SOME rule is applicable; includes transitive SOME support
	const DlCompletionTree* isSomeApplicable ( const TRole* R, BipolarPointer C ) const
		{ return R->isTransitive() ? isTSomeApplicable(R,C) : isNSomeApplicable(R,C); }

	//----------------------------------------------
	// Label access interface
	//----------------------------------------------

		/// RO access to a label
	const CGLabel& label ( void ) const { return Label; }
		/// RW access to a label
	CGLabel& label ( void ) { return Label; }

	// label iterators

		/// begin() iterator for a label with simple concepts
	const_label_iterator beginl_sc ( void ) const { return Label.begin_sc(); }
		/// end() iterator for a label with simple concepts
	const_label_iterator endl_sc ( void ) const { return Label.end_sc(); }
		/// begin() iterator for a label with complex concepts
	const_label_iterator beginl_cc ( void ) const { return Label.begin_cc(); }
		/// end() iterator for a label with complex concepts
	const_label_iterator endl_cc ( void ) const { return Label.end_cc(); }

		/// check whether node's label contains P
	bool isLabelledBy ( BipolarPointer p ) const { return Label.contains(p); }

	//----------------------------------------------
	// blocking interface
	//----------------------------------------------

	//  Blocked-By methods for different logics

		/// check blocking condition for SH logic
	bool isBlockedBy_SH ( const DlCompletionTree* p ) const { return B1(p); }
		/// check blocking condition for SHI logic
	bool isBlockedBy_SHI ( const DlCompletionTree* p ) const { return isCommonlyBlockedBy(p); }
		/// check blocking condition for SHIQ logic using optimised blocking
	bool isBlockedBy_SHIQ ( const DlCompletionTree* p ) const { return isCommonlyBlockedBy(p) && ( isCBlockedBy(p) || isABlockedBy(p) ); }

	// WARNING!! works only for blockable nodes
	// every non-root node will have first upcoming edge pointed to a parent

		/// return RO pointer to the parent node; WARNING: correct only for nodes with hasParent()==TRUE
	const DlCompletionTree* getParentNode ( void ) const { return (*beginp())->getArcEnd(); }
		/// return RW pointer to the parent node; WARNING: correct only for nodes with hasParent()==TRUE
	DlCompletionTree* getParentNode ( void ) { return (*beginp())->getArcEnd(); }

	//----------------------------------------------
	// managing AFFECTED flag
	//----------------------------------------------

		/// check whether node is affected by blocking-related changes
	bool isAffected ( void ) const { return affected; }
		/// set node (and all subnodes) affected
	void setAffected ( void )
	{
		// don't mark already affected, nominal or p-blocked nodes
		if ( isAffected() || isNominalNode() || isPBlocked() )
			return;
		affected = true;
		for ( const_edge_iterator q = begins(); q != ends(); ++q )
			(*q)->getArcEnd()->setAffected();
	}
		/// clear affected flag
	void clearAffected ( void ) { affected = false; }

	// just returns calculated values

		/// check if node is directly blocked
	bool isDBlocked ( void ) const { return Blocker != NULL && !pBlocked && dBlocked; }
		/// check if node is indirectly blocked
	bool isIBlocked ( void ) const { return Blocker != NULL && !pBlocked && !dBlocked; }
		/// check if node is purged (and so indirectly blocked)
	bool isPBlocked ( void ) const { return Blocker != NULL && pBlocked && !dBlocked; }
		/// check if node is blocked (d/i)
	bool isBlocked ( void ) const { return Blocker != NULL && !pBlocked; }
		/// check the legality of the direct block
	bool isIllegallyDBlocked ( void ) const { return isDBlocked() && Blocker->isBlocked(); }
		/// get node to which current one was merged
	const DlCompletionTree* resolvePBlocker ( void ) const
	{
		if ( isPBlocked() )
			return Blocker->resolvePBlocker();
		else
			return this;
	}
		/// get node to which current one was merged; fills DEP from pDep's
	DlCompletionTree* resolvePBlocker ( DepSet& dep )
	{
		if ( !isPBlocked() )
			return this;
		dep += pDep;
		return const_cast<DlCompletionTree*>(Blocker)->resolvePBlocker(dep);
	}
		/// check whether a node can block node P according to it's Init value
	bool canBlockInit ( const DlCompletionTree* p ) const { return canBlockInit(p->Init); }

		/// check whether the loop between a DBlocked NODE and it's parent blocked contains C
	bool isLoopLabelled ( BipolarPointer c )
	{
		assert ( isDBlocked() );
		if ( Blocker->isLabelledBy(c) )
			return true;
		for ( DlCompletionTree* p = getParentNode(); p->hasParent() && p != Blocker; p = p->getParentNode() )
			if ( p->isLabelledBy(c) )
				return true;
		return false;
	}

	//----------------------------------------------
	// re-building blocking hierarchy
	//----------------------------------------------

		/// set node blocked
	TRestorer* setBlocked ( const DlCompletionTree* blocker, bool permanently, bool directly )
	{
		TRestorer* ret = new UnBlock(this);
		Blocker = blocker;
		pBlocked = permanently;
		dBlocked = directly;
		logNodeBlocked();
		return ret;
	}
		/// mark node d-blocked
	TRestorer* setDBlocked ( const DlCompletionTree* blocker ) { return setBlocked ( blocker, false, true ); }
		/// mark node i-blocked
	TRestorer* setIBlocked ( const DlCompletionTree* blocker ) { return setBlocked ( blocker, false, false ); }
		/// mark node unblocked
	TRestorer* setUBlocked ( void ) { return setBlocked ( NULL, true, true ); }
		/// mark node purged
	TRestorer* setPBlocked ( const DlCompletionTree* blocker, const DepSet& dep )
	{
		TRestorer* ret = new UnBlock(this);
		Blocker = blocker;
		if ( isNominalNode() )
			pDep = dep;
		pBlocked = true;
		dBlocked = false;
		logNodeBlocked();
		return ret;
	}

	//----------------------------------------------
	//	checking edge labelling
	//----------------------------------------------

		/// check if parent arc is labelled by R; works only for blockable nodes
	bool isParentArcLabelled ( const TRole* R ) const
	{
		const_edge_iterator p = beginp(), p_end = endp();
		const DlCompletionTree* parent = (*p)->getArcEnd();
		for ( ; p < p_end; ++p )
			// make sure that the checking edge lead to parent (and not, e.g., a loop edge)
			if ( (*p)->getArcEnd() == parent && (*p)->isNeighbour(R) )
				return true;
		return false;
	}
		/// check if parent edge to P is labelled by R; return NULL if does not
	DlCompletionTreeArc* getParentEdgeLabelled ( const TRole* R, const DlCompletionTree* p ) const
		{ return getEdgeLabelled ( beginp(), endp(), R, p ); }
		/// check if successor edge to P is labelled by R; return NULL if does not
	DlCompletionTreeArc* getChildEdgeLabelled ( const TRole* R, const DlCompletionTree* p ) const
		{ return getEdgeLabelled ( begins(), ends(), R, p ); }
		/// check if edge to P given by ISPARENT is labelled by R; return NULL if does not
	DlCompletionTreeArc* getEdgeLabelled ( const TRole* R, const DlCompletionTree* p, bool isParent ) const
		{ return isParent ? getParentEdgeLabelled ( R, p ) : getChildEdgeLabelled ( R, p ); }

	//----------------------------------------------
	// inequality relation interface
	//----------------------------------------------

#ifdef RKG_IR_IN_NODE_LABEL
		/// init IR with given entry and dep-set; @return true if IR already has this label
	bool initIR ( unsigned int level, const DepSet& ds )
	{
		ConceptWDep C(level,ds);
		DepSet dummy;	// we don't need a clash-set here
		if ( inIRwithC ( C, dummy ) )
			return true;
		IR.add(C);
		return false;
	}
		/// check if the current node is in IR with NODE; if so, write the clash-set to DEP
	bool nonMergable ( const DlCompletionTree* node, DepSet& dep ) const;
		/// update IR of the current node with IR from NODE and additional dep-set; @return restorer
	TRestorer* updateIR ( const DlCompletionTree* node, const DepSet& toAdd );
#endif

	//----------------------------------------------
	// saving/restoring
	//----------------------------------------------

		/// check if node needs to be saved
	bool needSave ( unsigned int newLevel ) const { return curLevel < newLevel; }
		/// save node using internal stack
	void save ( unsigned int level )
	{
		save(saves.push());
		curLevel = level;
	}
		/// restore node from the topmost entry
	void restore ( void )
	{
		assert ( !saves.empty() );
		restore (saves.pop());
	}
		/// check if node needs to be restored
	bool needRestore ( unsigned int restLevel ) const { return curLevel > restLevel; }
		/// restore node to given level
	void restore ( unsigned int level ) { restore(saves.pop(level)); }

	// output

		/// log node information (number, i/d blockers, cached)
	void logNode ( void ) const { LL << getId(); logNodeBStatus(LL); }
		/// print body of the node (without neighbours)
	void PrintBody ( std::ostream& o ) const;
}; // DlCompletionTree

/*
 *  Implementation
 */

inline void DlCompletionTree :: init ( unsigned int level )
{
	flagDataNode = false;
	nominalLevel = BlockableLevel;
	curLevel = level;
	cached = false;
	affected = true;	// every (newly created) node can be blocked
	dBlocked = true;
	pBlocked = true;	// unused flag combination

	Label.init();
	Init = bpTOP;

	// node was used -- clear all previous content
	saves.clear();
#ifdef RKG_IR_IN_NODE_LABEL
	IR.clear();
#endif
	Parent.clear();
	Son.clear();
	Blocker = NULL;
	pDep.clear();
}

#endif // _DLCOMPLETIONTREE_H