//	UTreeDictionary.h
//	a binary search tree dictionary, indexed by a 32-bit key
#ifndef UTREEDICTIONARY_H
#define UTREEDICTIONARY_H

typedef void (*utdTraverseHook)(int, void *, void *);

class UTreeDictionary
{
	public:
	
		UTreeDictionary(int);
		~UTreeDictionary();
		
		bool Find(void *, int);
		status_t Insert(void *, int);
		void TraverseInOrder(utdTraverseHook, void *);
		
	private:
	
		uint8 *list;
		int dataSize;
		int nodeSize;
		int length;
		int root;
		
		int find(int, int);
		void insert(int, int, int);
		void traverseInOrder(int, utdTraverseHook, void *);
};

#endif 