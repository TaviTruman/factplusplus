/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2003-2008 by Dmitry Tsarkov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "dlDag.h"
#include "dlCompletionGraph.h"
#include "Reasoner.h"

// statistic for calling blocking
unsigned long tries[6], fails[6], nSucc, blockIndex;

void printBlockingStat ( std::ostream& o )
{
	if ( *tries == 0 )	// nothing to inform
		return;

	// else -- print precize statistics
	o << "\nThere were made " << *tries << " blocking tests of which "
	  << nSucc << " successfull.\nBlocking rules failure statistic:";
	for ( uint i = 0; i < 6; ++i )
	{
		if ( i != 0 )
			o << ",";
		o << " " << fails[i] << "/" << tries[i];
	}
}

void clearBlockingStat ( void )
{
	for ( int i = 5; i >= 0; --i )
		tries[i] = fails[i] = 0;
	nSucc = blockIndex = 0;
}

// universal Blocked-By method
bool
DlCompletionGraph :: isBlockedBy ( const DlCompletionTree* node, const DlCompletionTree* blocker ) const
{
	// nominal nodes can't neither be nor became blocked
	if ( node->isNominalNode() || blocker->isNominalNode() )
		return false;

	// cached node can't be a blocker
	if ( blocker->isCached() )
		return false;

	// easy check: Init is not in the label if a blocker
	if ( !blocker->canBlockInit(node) )
		return false;

	bool ret;
	if ( sessionHasInverseRoles )	// subset blocking
	{
		if ( sessionHasNumberRestrictions )	// I+F -- optimised blocking
			ret = node->isBlockedBy_SHIQ(blocker);
		else								// just I -- equality blocking
			ret = node->isBlockedBy_SHI(blocker);
	}
	else
		ret = node->isBlockedBy_SH(blocker);

	if ( ret )
		++nSucc;
	return ret;
}

// Blocked-by implementation

bool DlCompletionTree :: isCommonlyBlockedBy ( const DlCompletionTree* p ) const
{
	// common B1:
	if ( !B1(p) )
		return false;

	for ( const_label_iterator q = p->beginl_cc(), q_end = p->endl_cc(); q < q_end; ++q )
	{
		BipolarPointer bp = q->bp();
		const DLVertex& v = (*pDLHeap)[bp];

		if ( v.Type() == dtForall && isPositive(bp) )
		{	// (all S C) \in L(w')
			if ( !B2 ( v, bp ) )
				return false;
		}
	}

	return true;
}

bool DlCompletionTree :: isABlockedBy ( const DlCompletionTree* p ) const
{
	// current = w; p = w'; parent = v
#ifdef ENABLE_CHECKING
	assert ( hasParent () );	// there exists v
#endif

	// B3,B4
	for ( const_label_iterator q = p->beginl_cc(), q_end = p->endl_cc(); q < q_end; ++q )
	{
		BipolarPointer bp = q->bp();
		const DLVertex& v = (*pDLHeap)[bp];

		if ( v.Type() == dtForall && isNegative(bp) )
		{	// (some T E) \in L(w')
			if ( !B4 ( p, 1, v.getRole(), inverse(v.getC()) ) )
				return false;
		}
		else if ( v.Type() == dtLE )
		{
			if ( isPositive(bp) )
			{	// (<= n S C) \in L(w')
				if ( !B3 ( p, v.getNumberLE(), v.getRole(), v.getC() ) )
					return false;
			}
			else
			{	// (>= m T E) \in L(w')
				if ( !B4 ( p, v.getNumberGE(), v.getRole(), v.getC() ) )
					return false;
			}
		}
	}

	// all other is OK -- done;
	return true;
}

bool DlCompletionTree :: isCBlockedBy ( const DlCompletionTree* p ) const
{
	// current = w; p = w'; parent = v
#ifdef ENABLE_CHECKING
	assert ( hasParent () );	// there exists v
#endif

	// B5
	const_label_iterator q, q_end;
	for ( q = p->beginl_cc(), q_end = p->endl_cc(); q < q_end; ++q )
	{
		BipolarPointer bp = q->bp();
		const DLVertex& v = (*pDLHeap)[bp];

		if ( v.Type() == dtLE && isPositive(bp) )
		{	// (<= n T E) \in L(w')
			if ( !B5 ( v.getRole(), v.getC() ) )
				return false;
		}
	}

	// B6
	const DlCompletionTree* par = getParentNode();
	for ( q = par->beginl_cc(), q_end = par->endl_cc(); q < q_end; ++q )
	{
		BipolarPointer bp = q->bp();
		const DLVertex& v = (*pDLHeap)[bp];

		if ( v.Type() == dtLE && isNegative(bp) )
		{	// (>= m U F) \in L(v)
			if ( !B6 ( v.getRole(), v.getC() ) )
				return false;
		}
	}

	return true;
}

//----------------------------------------------------------------------
//--   B1 to B6 conditions implementation
//--  WARNING!! 19-06-2005. All blockable nodes has the only parent
//--    (with probably several links to it). So we should check all of them
//----------------------------------------------------------------------

#define TRY_B(i)		++tries[i-1]
#define FAIL_B(i)		++fails[i-1]

	/// check if B1 holds for a given vertex (p is a candidate for blocker)
bool DlCompletionTree :: B1 ( const DlCompletionTree* p ) const
{
	TRY_B(1);

	if ( Label <= p->Label )
		return true;

	FAIL_B(1);
	return false;
}

	/// check if B2 holds for (AS C) a simple automaton A for S
bool DlCompletionTree :: B2 ( const RoleAutomaton& A, BipolarPointer C ) const
{
	const DlCompletionTree* parent = getParentNode();
	const CGLabel& parLab = parent->label();
	RATransition* trans = *A.begin(0);
	TRY_B(2);

	for ( const_edge_iterator p = beginp(), p_end = endp(); p < p_end; ++p )
		if ( !(*p)->isIBlocked() && (*p)->getArcEnd() == parent && trans->applicable((*p)->getRole()) )
		{
			if ( !parLab.contains(C) )
			{
				FAIL_B(2);
				return false;
			}
			else
				return true;
		}

	return true;
}

	/// check if B2 holds for C=(AS{n} X) a complex automaton A for S
bool DlCompletionTree :: B2 ( const RoleAutomaton& A, BipolarPointer C, RAState n ) const
{
	const DlCompletionTree* parent = getParentNode();
	const CGLabel& parLab = parent->label();
	RoleAutomaton::const_trans_iterator q, end = A.end(n);
	TRY_B(2);

	for ( const_edge_iterator p = beginp(), p_end = endp(); p < p_end; ++p )
	{
		if ( (*p)->isIBlocked() || (*p)->getArcEnd() != parent )
			continue;
		const TRole* R = (*p)->getRole();

		for ( q = A.begin(n); q != end; ++q )
			if ( (*q)->applicable(R) )
				if ( !parLab.containsCC(C-n+(*q)->final()) )
				{
					FAIL_B(2);
					return false;
				}
	}

	return true;
}

	/// check if B3 holds for (<= n S.C)\in w' (p is a candidate for blocker)
bool DlCompletionTree :: B3 ( const DlCompletionTree* p, unsigned int n, const TRole* S, BipolarPointer C ) const
{
#ifdef ENABLE_CHECKING
	assert ( hasParent () );	// safety check
#endif
	TRY_B(3);

	bool ret;
	// if (<= n S C) \in L(w') then
	// a) w is an inv(S)-succ of v or
	if ( !isParentArcLabelled(S) )
		ret = true;
	// b) w is an inv(S) succ of v and ~C\in L(v) or
	else if ( getParentNode()->isLabelledBy(inverse(C)) )
		ret = true;
	// c) w is an inv(S) succ of v and C\in L(v)...
	else if ( !getParentNode()->isLabelledBy(C) )
		ret = false;
	else
	{	// ...and <=n-1 S-succ. z with C\in L(z)
		register unsigned int m = 0;
		for ( const_edge_iterator q = p->begins(), q_end = p->ends(); q < q_end; ++q )
			if ( (*q)->isNeighbour(S) && (*q)->getArcEnd()->isLabelledBy(C) )
				++m;

		ret = ( m < n );
	}

	if ( !ret )
		FAIL_B(3);

	return ret;
}

	/// check if B4 holds for (>= m T.E)\in w' (p is a candidate for blocker)
bool DlCompletionTree :: B4 ( const DlCompletionTree* p, unsigned int m, const TRole* T, BipolarPointer E ) const
{
#ifdef ENABLE_CHECKING
	assert ( hasParent () );	// safety check
#endif
	TRY_B(4);

	// if (>= m T E) \in L(w') then
	// b) w is an inv(T) succ of v and E\in L(v) and m == 1 or
	if ( isParentArcLabelled(T) && m == 1 && getParentNode()->isLabelledBy(E) )
		return true;

	// a) w' has at least m T-succ z with E\in L(z)
	// check all sons
	register unsigned int n = 0;
	for ( const_edge_iterator q = p->begins(), q_end = p->ends(); q < q_end; ++q )
		if ( (*q)->isNeighbour(T) && (*q)->getArcEnd()->isLabelledBy(E) )
			if ( ++n >= m )		// check if node has enough successors
				return true;

	// rule check fails
	FAIL_B(4);
	return false;
}

	/// check if B5 holds for (<= n T.E)\in w'
bool DlCompletionTree :: B5 ( const TRole* T, BipolarPointer E ) const
{
#ifdef ENABLE_CHECKING
	assert ( hasParent () );	// safety check
#endif
	TRY_B(5);

	// if (<= n T E) \in L(w'), then
	// either w is not an inv(T)-successor of v...
	if ( !isParentArcLabelled(T) )
		return true;
	// or ~E \in L(v)
	if ( getParentNode()->isLabelledBy ( inverse(E) ) )
		return true;

	FAIL_B(5);
	return false;
}

	/// check if B6 holds for (>= m U.F)\in v
bool DlCompletionTree :: B6 ( const TRole* U, BipolarPointer F ) const
{
#ifdef ENABLE_CHECKING
	assert ( hasParent () );	// safety check
#endif
	TRY_B(6);

	// if (>= m U F) \in L(v), and
	// w is U-successor of v...
	if ( !isParentArcLabelled(U->inverse()) )
		return true;
	// then ~F\in L(w)
	if ( isLabelledBy ( inverse(F) ) )
		return true;

	FAIL_B(6);
	return false;
}

#undef TRY_B
#undef FAIL_B

//----------------------------------------------------------------------
//--   changing blocked status
//----------------------------------------------------------------------

void DlCompletionGraph :: detectBlockedStatus ( DlCompletionTree* node )
{
	DlCompletionTree* p = node;
	bool wasBlocked = node->isBlocked();
	bool wasDBlocked = node->isDBlocked();

	while ( p->hasParent() && p->isBlockableNode() && p->isAffected() )
	{
		findDBlocker(p);
		if ( p->isBlocked() )
			return;
		p = p->getParentNode();
	}
	p->clearAffected();
	if ( wasBlocked && !node->isBlocked() )
		unblockNode ( node, wasDBlocked );
}

void DlCompletionGraph :: unblockNode ( DlCompletionTree* node, bool wasDBlocked )
{
	if ( node->isPBlocked() || !node->isBlockableNode() )
		return;
	if ( !wasDBlocked )	// if it was DBlocked -- findDBlocker() made it
		saveRareCond(node->setUBlocked());
	pReasoner->repeatUnblockedNode(node,wasDBlocked);
	unblockNodeChildren(node);
}

void DlCompletionGraph :: findDAncestorBlocker ( DlCompletionTree* node )
{
	register const DlCompletionTree* p = node;

	while ( p->hasParent() && p->isBlockableNode() )
	{
		p = p->getParentNode();

		if ( isBlockedBy ( node, p ) )
		{
			setNodeDBlocked ( node, p );
			return;
		}
	}
}

void DlCompletionGraph :: findDAnywhereBlocker ( DlCompletionTree* node )
{
	for ( const_iterator q = begin(), q_end = end(); q < q_end && *q != node; ++q )
	{
		const DlCompletionTree* p = *q;

		// node was merge to smth with the larger ID or is blocked itself
		if ( p->isBlocked() || p->isPBlocked() )
			continue;

		if ( isBlockedBy ( node, p ) )
		{
			setNodeDBlocked ( node, p );
			return;
		}
	}
}
