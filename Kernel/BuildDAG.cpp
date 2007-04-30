/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2003-2007 by Dmitry Tsarkov

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

// This file contains methods for creating DAG representation of KB

#include <set>
#include "dlTBox.h"

void TBox :: buildDAG ( void )
{
	if ( useRelevantOnly )
	{
		if ( queryPointer[1] )
			addConceptToHeap ( queryPointer[1] );

		if ( queryPointer[0] )
			addConceptToHeap ( queryPointer[0] );
	}
	else
	{
		for ( c_const_iterator pc = c_begin(); pc != c_end(); ++pc )
			addConceptToHeap (*pc);
		for ( i_const_iterator pi = i_begin(); pi != i_end(); ++pi )
			addConceptToHeap (*pi);
	}

	// build all GCIs
	T_G = tree2dag(Axioms.getGCI());
}

/// register data expression in the DAG
BipolarPointer TBox :: addDataExprToHeap ( TDataEntry* p )
{
	if ( isValid(p->getBP()) )	// already registered value
		return p->getBP();

	// determine the type of an entry
	DagTag dt = p->isBasicDataType() ? dtDataType : p->isDataValue() ? dtDataValue : dtDataExpr;
	BipolarPointer hostBP = bpTOP;

	// register host type first (if any)
	if ( p->getType() != NULL )
		hostBP = addDataExprToHeap(const_cast<TDataEntry*>(p->getType()));

	// create new DAG entry for the data value
	DLVertex* ver = new DLVertex ( dt, hostBP );
	ver->setConcept(p);
	p->setBP(DLHeap.directAdd(ver));

	return p->getBP();
}

void TBox :: addConceptNameToHeap ( TConcept* pConcept, bool isCycled )
{
	assert ( pConcept != NULL );	// safety check
//	assert ( isRegisteredConcept (pConcept) );	// FIXME!! return it with details about indiv

	// check if the concept is already there
	if ( isPositive ( pConcept->pName ) )
	{	// it was a cycled one; now it is not a cycle
		assert ( !isCycled );
		DLHeap[pConcept->pName]. addChild ( pConcept->pBody );
		// FIXME!! the following line should take into account terminal nodes
		// but the mis-optimisation is to big. need investigation
//		DLHeap[pConcept->pName].initStat(DLHeap);
		// mark concept as a complete
		pConcept->setIncomplete(false);
		return;
	}

	// choose proper tag by concept
	DagTag tag = pConcept->isPrimitive() ?
		(pConcept->isSingleton() ? dtPSingleton : dtPConcept):
		(pConcept->isSingleton() ? dtNSingleton : dtNConcept);
	// new concept's addition
	DLVertex* ver = isCycled ? new DLVertex(tag) : new DLVertex ( tag, pConcept->pBody );
	ver->setConcept(pConcept);
	pConcept->pName = DLHeap.directAdd(ver);
}

void TBox :: addConceptToHeap ( TConcept* pConcept )
{
	assert ( pConcept != NULL );

	// set of named concepts that are processing now
	static std::set<TConcept*> inProcess;

	// if we found a cycle
	if ( inProcess.find (pConcept) != inProcess.end () )
	{
		// create an fake entry of a concept
		addConceptNameToHeap ( pConcept, true );
		// inform about cyclic dependence
		if ( verboseOutput )
			std::cerr << "\ncyclic dependences via concept \"" << pConcept->getName() << '\"';
		// mark concept as incomplete
		pConcept->setIncomplete(true);
		return;
	}

	// add concept in processing
	inProcess.insert (pConcept);

	if ( pConcept->Description != NULL )	// complex concept
		pConcept->pBody = tree2dag(pConcept->Description);
	else	//-- nothing to do;
	{	// only primivive versions here
		assert ( !pConcept->isNonPrimitive() );
		pConcept->pBody = bpTOP;
	}

	// add concept name to heap if it was not a cycle
	addConceptNameToHeap (pConcept, false);

	// remove processed concept from set
	inProcess.erase (pConcept);
}

BipolarPointer TBox :: tree2dag ( const DLTree* t )
{
	if ( t == NULL )
		return bpINVALID;	// invalid value

	const TLexeme& cur = t->Element();

	switch ( cur.getToken() )
	{
	case BOTTOM:	// it is just !top
		return bpBOTTOM;
	case TOP:	// the 1st node
		return bpTOP;
	case DATAEXPR:	// data-related expression
		return addDataExprToHeap ( static_cast<TDataEntry*>(cur.getName()) );
	//FIXME!!! return here when it would be difference between CN/IN
	case NAME:	// it is a concept ID
		return concept2dag(getConcept(cur.getName()));

	case NOT:
		return inverse ( tree2dag ( t->Left() ) );

	case AND:
	{
		DLVertex* ret = new DLVertex(dtAnd);
		// fills it with multiply data
		if ( fillANDVertex ( ret, t ) )
		{
			// sorts are broken now (see bTR11)
			useSortedReasoning = false;
			delete ret;
			return bpBOTTOM;	// clash found => bottom
		}
		else	// AND vertex
			if ( (ret->end() - ret->begin() ) == 1 )	// and(C) = C
			{
				BipolarPointer p = *ret->begin();
				delete ret;
				return p;
			}
			else
				return DLHeap.add(ret);
	}

	case FORALL:
		return forall2dag ( role2dag(t->Left()), tree2dag(t->Right()) );

	case REFLEXIVE:
		return inverse ( DLHeap.add ( new DLVertex ( dtIrr, role2dag(t->Left()) ) ) );

	case LE:
		return atmost2dag ( cur.getData(), role2dag(t->Left()), tree2dag(t->Right()) );

	default:
		assert ( isSNF(t) );	// safety check
		assert (0);				// extra safety check ;)
		return bpINVALID;
	}
}

BipolarPointer TBox :: forall2dag ( const TRole* R, BipolarPointer C )
{
	if ( R->isDataRole() )
		return dataForall2dag(R,C);

	// create \all R.C == \all R{0}.C
	BipolarPointer ret = DLHeap.add ( new DLVertex ( dtForall, 0, R, C ) );

	if ( R->isSimple() )	// don't care about the rest
		return ret;

	// check if the concept is not last
	if ( !DLHeap.isLast(ret) )
		return ret;		// all sub-roles were added before

	// have appropriate concepts for all the automata states
	for ( unsigned int i = 1; i < R->getAutomaton().size(); ++i )
		DLHeap.directAddAndCache ( new DLVertex ( dtForall, i, R, C ) );

	return ret;
}

BipolarPointer
TBox :: dataForall2dag ( const TRole* R, BipolarPointer C )
{
	return DLHeap.add ( new DLVertex ( dtForall, 0, R, C ) );
}

BipolarPointer TBox :: atmost2dag ( unsigned int n, const TRole* R, BipolarPointer C )
{
	if ( R->isDataRole() )
		return dataAtMost2dag(n,R,C);

	BipolarPointer ret = DLHeap.add ( new DLVertex ( dtLE, n, R, C ) );

	// check if the concept is not last
	if ( !DLHeap.isLast(ret) )
		return ret;		// all elements were added before

	// create entries for the transitive sub-roles
	for ( unsigned int m = n-1; m > 0; --m )
		DLHeap.directAddAndCache ( new DLVertex ( dtLE, m, R, C ) );

	return ret;
}

BipolarPointer
TBox :: dataAtMost2dag ( unsigned int n, const TRole* R, BipolarPointer C )
{
	return DLHeap.add ( new DLVertex ( dtLE, n, R, C ) );
}

const TRole* TBox :: role2dag ( const DLTree* t )
{
	TRole* r = resolveRole(t);
	return r ? r->resolveSynonym() : NULL;
}

bool TBox :: fillANDVertex ( DLVertex* v, const DLTree* t )
{
	if ( t->Element().getToken() == AND )
		return fillANDVertex ( v, t->Left() ) || fillANDVertex ( v, t->Right() );
	else
		return v->addChild ( tree2dag(t) );
}
