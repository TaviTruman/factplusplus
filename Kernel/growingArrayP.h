/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2005-2007 by Dmitry Tsarkov

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

#ifndef _GROWINGARRAYP_H
#define _GROWINGARRAYP_H

#include <vector>

/**
 *	Generic class for structures which creates elements (by pointers) and re-use
 *	them (does not delete things).  Derived types may add operations.
 */
template<class T>
class growingArrayP
{
private:	// static members
		/// initial size of heap
	static const unsigned int initSize = 8;

protected:	// typedefs
		/// type of the heap
	typedef std::vector<T*> baseType;

public:		// typedefs
		/// heap's iterator
	typedef typename baseType::iterator iterator;
		/// heap's const iterator
	typedef typename baseType::const_iterator const_iterator;

protected:	// members
		/// heap itself
	baseType Base;
		/// index of the next unallocated entry
	unsigned int last;

private:	// prevent from copying
		/// no copy c'tor
	growingArrayP ( const growingArrayP& );
		/// no assignment
	growingArrayP& operator= ( const growingArrayP& );

protected:	// methods
		/// tunable method for creating new object
	virtual T* createNew ( void ) { return new T; }
		/// init vector [B,E) with new objects T
	void initArray ( iterator b, iterator e )
	{
		for ( iterator p = b; p != e; ++p )
			*p = createNew();
	}
		/// increase heap size
	void grow ( void )
	{
		unsigned int size = Base.size();
		Base.resize(size?size*2:1);
		initArray ( Base.begin()+size, Base.end() );
	}
		/// ensure that size of vector is enough to fit the last element
	void ensureHeapSize ( void )
	{
		if ( last >= Base.size() )
			grow();
	}
		/// ensure that size of vector is enough to keep N elements
	void ensureHeapSize ( unsigned int n )
	{
		while ( n >= Base.size() )
			grow();
	}

public:		// interface
		/// c'tor: make SIZE objects
	growingArrayP ( unsigned int size = growingArrayP::initSize ) : Base(size), last(0)
	{
		initArray ( Base.begin(), Base.end() );
	}

		/// d'tor: delete all allocated objects
	virtual ~growingArrayP ( void )
	{
		for ( iterator p = Base.begin(); p != Base.end(); ++p )
			delete *p;
	}

		/// check if heap is empty
	bool empty ( void ) const { return last == 0; }
		/// mark all array elements as unused
	virtual void clear ( void ) { last = 0; }
}; // growingArrayP

#endif
