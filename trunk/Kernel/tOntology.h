/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2008-2009 by Dmitry Tsarkov

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

#ifndef TONTOLOGY_H
#define TONTOLOGY_H

#include <vector>
#include "tDLAxiom.h"

/// define ontology as a set of axioms
class TOntology
{
protected:	// types
		/// base type for the set of axioms
	typedef std::vector<TDLAxiom*> TAxiomArray;
		/// RW iterator ovet base type
	typedef TAxiomArray::iterator iterator;

protected:	// members
		/// all the axioms
	TAxiomArray Axioms;
		/// id to be given to the next axiom
	unsigned int axiomId;
		/// index of the 1st unprocessed axiom
	size_t axiomToProcess;

protected:	// methods

public:		// interface
		/// empty c'tor
	TOntology ( void ) : axiomId(0), axiomToProcess(0) {}
		/// d'tor
	~TOntology ( void ) { clear(); }

		/// @return true iff the ontology was changed since its last load
	bool isChanged ( void ) const { return axiomToProcess != Axioms.size(); }

		/// add given axiom to the ontology
	void add ( TDLAxiom* p )
	{
		p->setId(++axiomId);
		Axioms.push_back(p);
	}
		/// load ontology to a given KB
	void load ( TBox& kb )
	{
		for ( iterator p = Axioms.begin()+axiomToProcess, p_end = Axioms.end(); p < p_end; ++p )
			(*p)->load(kb);
		axiomToProcess = Axioms.size();
	}
		/// clear the ontology
	void clear ( void )
	{
		for ( iterator p = Axioms.begin(), p_end = Axioms.end(); p < p_end; ++p )
			delete *p;
		Axioms.clear();
		axiomToProcess = 0;
	}
}; // TOntology

#endif