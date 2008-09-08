/*

	Filename:	HSStack.h
	Contents:	HSStack-class declaration and definition.
	Author:		Heikki Suhonen

*/

// This is a simple generic stack. It should not be needed
// after we get the STL working.

template<class T> class HSStack {
	T*	v;
	T*	p;
	int sz;

public:
		HSStack(int s=20) { v = p = new T[sz=s]; }
		~HSStack() { delete[] v; }

void	push(T a) { ( (p-v<sz) ? (*p++ = a) : (a=a)); }
T		pop() { return (v<p ? *--p : NULL ); }

int		size() const { return p-v; }
};
