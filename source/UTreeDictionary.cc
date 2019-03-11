//	UTreeDictionary.cc

#include "XPM.h"
#include "UTreeDictionary.h"

#define		UTD_LENGTH_UNIT			256

typedef struct
{
	int key;
	int next;
	int left;
	int right;
}
tree_record;

UTreeDictionary::UTreeDictionary(int datSize)
{
	uint8 *addr;
	tree_record *t;
	int i;
	
	dataSize = datSize;	
	nodeSize = dataSize+sizeof(tree_record);
	length = UTD_LENGTH_UNIT;
	list = (uint8 *)malloc(length*nodeSize);
	addr = list;
	for (i = 0; i < length-1; i++)
	{
		t = (tree_record *)addr;
		t->next = i+1;
		addr += nodeSize;
	}
	t = (tree_record *)addr;
	t->next = 0;
	root = 0;
}

UTreeDictionary::~UTreeDictionary()
{
	free(list);
}

bool UTreeDictionary::Find(void *data, int key)
{
	uint8 *addr;
	int index = find(key,root);
	
	if (index)
	{
		if (data)
		{
			addr = list+index*nodeSize;
			memcpy(data,addr+sizeof(tree_record),dataSize);
		}
		return true;
	}
	else
		return false;
}

int UTreeDictionary::find(int key, int index)
{
	tree_record *t;
	
	if (index)
	{
		t = (tree_record *)(list+index*nodeSize);
		if (t->key == key)
			return index;
		else if (key < t->key)
			return find(key,t->left);
		else
			return find(key,t->right);
	}
	else
		return 0;
}

status_t UTreeDictionary::Insert(void *data, int key)
{
	uint8 *addr;
	tree_record *t, *head;
	int i, oldLength, free;
	
	head = (tree_record *)list;
	if (!head->next)					// no free nodes; grow the list
	{
		oldLength = length;
		head->next = oldLength;
		length += UTD_LENGTH_UNIT;
		list = (uint8 *)realloc(list,length*nodeSize);
		head = (tree_record *)list;
		addr = list+oldLength*nodeSize;
		for (i = oldLength; i < length-1; i++)
		{
			t = (tree_record *)addr;
			t->next = i+1;
			addr += nodeSize;
		}
		t = (tree_record *)addr;
		t->next = 0;
	}
	free = head->next;
	addr = list+free*nodeSize;
	memcpy(addr+sizeof(tree_record),data,dataSize);
	t = (tree_record *)addr;
	head->next = t->next;
	t->next = 0;
	t->left = 0;
	t->right = 0;
	t->key = key;
	if (!root)
		root = free;
	else
		insert(key,root,free);
		
	return B_OK;
}

void UTreeDictionary::insert(int key, int index, int newIndex)
{
	tree_record *t;
	
	t = (tree_record *)(list+index*nodeSize);
	if (key < t->key)
	{
		if (t->left)
			insert(key,t->left,newIndex);
		else
			t->left = newIndex;
	}
	else
	{
		if (t->right)
			insert(key,t->right,newIndex);
		else
			t->right = newIndex;
	}
}

void UTreeDictionary::TraverseInOrder(utdTraverseHook hook, void *arg)
{
	traverseInOrder(root,hook,arg);
	if (!hook)
		printf("\n");
}

void UTreeDictionary::traverseInOrder(int index, utdTraverseHook hook, void *arg)
{
	tree_record *t;
	uint8 *addr;
	
	if (index)
	{
		addr = list+index*nodeSize;
		t = (tree_record *)addr;
		traverseInOrder(t->left,hook,arg);
		if (!hook)
			printf("%d ",t->key);
		else
			hook(t->key,addr+sizeof(tree_record),arg);
		traverseInOrder(t->right,hook,arg);
	}
}
