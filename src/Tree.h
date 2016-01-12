/************************************************************************
** FILE NAME..... Tree.h
**
** (c) COPYRIGHT
**
** FUNCTION......... A subset of STL tree implementation
**
** NOTES............ please add 
**
** ASSUMPTIONS......
**
** RESTRICTIONS.....
**
** LIMITATIONS......
**
** DEVIATIONS.......
**
** RETURN VALUES.... 0  - successful
**                   !0 - error
**
** AUTHOR(S)........ Michael Q Yan
**
** CHANGES:
**
************************************************************************/

#ifndef _AT_TREE_H
#define _AT_TREE_H

typedef at_seqnum_t key_type;
typedef struct _Rb_tree_node_base* _Base_ptr;
typedef struct _Rb_tree_node* _Link_type;
typedef _Link_type Rb_tree_node;
typedef unsigned long size_type;

// User data attached to tree node
struct Val_pair
{
	Val_pair(key_type k, void* p) : _mKey(k), _mpBlock(p) {}

	key_type _mKey;
	void*    _mpBlock;
};

typedef bool (*_Compare)(key_type, key_type);

enum _Rb_tree_color { _M_red = false, _M_black = true };

// This structure encapsulates generic RB tree node properties
struct _Rb_tree_node_base
{
	_Rb_tree_color 	_M_color; 
	_Base_ptr 		_M_parent;
	_Base_ptr 		_M_left;
	_Base_ptr 		_M_right;

	static _Base_ptr _S_minimum(_Base_ptr __x)
	{
		while (__x->_M_left != 0) __x = __x->_M_left;
		return __x;
	}

	static _Base_ptr _S_maximum(_Base_ptr __x)
	{
		while (__x->_M_right != 0) __x = __x->_M_right;
		return __x;
	}
};

// Basic RB tree node + key/data pointer
struct _Rb_tree_node : public _Rb_tree_node_base
{
	_Rb_tree_node() : _M_value_field(0,0) {}

	void* GetValue()  { return _M_value_field._mpBlock; }
	//key_type GetKey() { return _M_value_field._mKey; }

	Val_pair _M_value_field;
	//key_type _mKey;
};

#define TREE_NODE_SIZE (sizeof(_Rb_tree_node))

// RB tree with sentinel header node
struct Rb_tree
{
	typedef Val_pair value_type;

	//Rb_tree(const _Compare& __comp)
	//: _M_node_count(0), _M_key_compare(__comp) 
	Rb_tree() : _M_node_count(0)
	{
		_M_header = new _Rb_tree_node;
		_M_empty_initialize();
	}

	// Insert/erase.
	//bool insert_unique(const value_type& __x);
	inline bool insert_unique(key_type __key, void* __block, void* __mem);

	_Link_type& _M_root() const { return (_Link_type&) _M_header->_M_parent; }

	static _Link_type& _S_left(_Link_type __x) { return (_Link_type&)(__x->_M_left); }

	static _Link_type& _S_right(_Link_type __x) { return (_Link_type&)(__x->_M_right); }

	_Link_type& _M_leftmost() const { return (_Link_type&) _M_header->_M_left; }

	_Link_type& _M_rightmost() const { return (_Link_type&) _M_header->_M_right; }

	_Link_type& begin() { return _M_leftmost(); }

	_Link_type& end() { return _M_header; }
	
	static _Rb_tree_color& _S_color(_Link_type __x) { return __x->_M_color; }

	inline _Link_type find(const key_type& __x);
	inline void erase(_Link_type& __node);

	size_type size() const { return _M_node_count; }

protected:
	//_Link_type _M_create_node(const value_type& __x)
	_Link_type _M_create_node(key_type __key, void* __block, void* __mem)
	{
		//_Link_type __tmp = _M_get_node();
		// Use the memory pointed by mpBlock as tree node storage
		_Link_type __tmp = (_Link_type)__mem;

		//__tmp->_M_value_field.mKey = __x.mKey;
		//__tmp->_M_value_field.mpBlock = __x.mpBlock;
		__tmp->_M_value_field._mKey = __key;
		__tmp->_M_value_field._mpBlock = __block;
		return __tmp;
	}

private:
	inline _Link_type _M_insert(_Base_ptr __x, _Base_ptr __y, key_type __key, void* __block, void* __mem);

	void _M_empty_initialize() 
	{
		// used to distinguish header from __root, in iterator.operator++
		_S_color(_M_header) = _M_red;
		_M_root() = 0;
		_M_leftmost() = _M_header;
		_M_rightmost() = _M_header;
	}

	_Rb_tree_node* _M_header;
	size_type      _M_node_count; // keeps track of size of tree
	//_Compare       _M_key_compare;
};

//////////////////////////////////////////////////////////////////
// Helper
//////////////////////////////////////////////////////////////////
inline void _M_decrement(_Link_type& ipNode)
{
	if (ipNode->_M_color == _M_red && ipNode->_M_parent->_M_parent == ipNode)
	{
		ipNode = (_Link_type)ipNode->_M_right;
	}
	else if (ipNode->_M_left != 0) 
	{
		_Base_ptr __y = ipNode->_M_left;
		while (__y->_M_right != 0)
		{
			__y = __y->_M_right;
		}
		ipNode = (_Link_type)__y;
	}
	else 
	{
		_Base_ptr __y = ipNode->_M_parent;
		while (ipNode == __y->_M_left) 
		{
			ipNode = (_Link_type)__y;
			__y = __y->_M_parent;
		}
		ipNode = (_Link_type)__y;
	}
}

inline void 
_Rb_tree_rotate_left(_Rb_tree_node_base* __x, _Rb_tree_node_base*& __root)
{
	_Rb_tree_node_base* __y = __x->_M_right;
	__x->_M_right = __y->_M_left;
	if (__y->_M_left !=0)
	{
		__y->_M_left->_M_parent = __x;
	}
	__y->_M_parent = __x->_M_parent;

	if (__x == __root)
	{
		__root = __y;
	}
	else if (__x == __x->_M_parent->_M_left)
	{
		__x->_M_parent->_M_left = __y;
	}
	else
	{
		__x->_M_parent->_M_right = __y;
	}
	__y->_M_left = __x;
	__x->_M_parent = __y;
}

inline void 
_Rb_tree_rotate_right(_Rb_tree_node_base* __x, _Rb_tree_node_base*& __root)
{
	_Rb_tree_node_base* __y = __x->_M_left;
	__x->_M_left = __y->_M_right;
	if (__y->_M_right != 0)
	{
		__y->_M_right->_M_parent = __x;
	}
	__y->_M_parent = __x->_M_parent;

	if (__x == __root)
	{
		__root = __y;
	}
	else if (__x == __x->_M_parent->_M_right)
	{
		__x->_M_parent->_M_right = __y;
	}
	else
	{
		__x->_M_parent->_M_left = __y;
	}
	__y->_M_right = __x;
	__x->_M_parent = __y;
}

inline _Rb_tree_node_base*
_Rb_tree_rebalance_for_erase(_Rb_tree_node_base* __z, 
							_Rb_tree_node_base*& __root,
							_Rb_tree_node_base*& __leftmost,
							_Rb_tree_node_base*& __rightmost)
{
	_Rb_tree_node_base* __y = __z;
	_Rb_tree_node_base* __x = 0;
	_Rb_tree_node_base* __x_parent = 0;
	if (__y->_M_left == 0)     // __z has at most one non-null child. y == z.
	{
		__x = __y->_M_right;     // __x might be null.
	}
	else
	{
		if (__y->_M_right == 0)  // __z has exactly one non-null child. y == z.
		{
			__x = __y->_M_left;    // __x is not null.
		}
		else 
		{
			// __z has two non-null children.  Set __y to
			__y = __y->_M_right;   //   __z's successor.  __x might be null.
			while (__y->_M_left != 0)
			{
				__y = __y->_M_left;
			}
			__x = __y->_M_right;
		}
	}
	if (__y != __z) 
	{
		// relink y in place of z.  y is z's successor
		__z->_M_left->_M_parent = __y; 
		__y->_M_left = __z->_M_left;
		if (__y != __z->_M_right) 
		{
			__x_parent = __y->_M_parent;
			if (__x)
			{
				__x->_M_parent = __y->_M_parent;
			}
			__y->_M_parent->_M_left = __x;   // __y must be a child of _M_left
			__y->_M_right = __z->_M_right;
			__z->_M_right->_M_parent = __y;
		}
		else
		{
			__x_parent = __y;  
		}
		if (__root == __z)
		{
			__root = __y;
		}
		else if (__z->_M_parent->_M_left == __z)
		{
			__z->_M_parent->_M_left = __y;
		}
		else 
		{
			__z->_M_parent->_M_right = __y;
		}
		__y->_M_parent = __z->_M_parent;
		
		//std::swap(__y->_M_color, __z->_M_color);
		_Rb_tree_color __tmp = __y->_M_color;
		__y->_M_color = __z->_M_color;
		__z->_M_color = __tmp;

		__y = __z;
		// __y now points to node to be actually deleted
	}
	else 
	{                        // __y == __z
		__x_parent = __y->_M_parent;
		if (__x) 
		{
			__x->_M_parent = __y->_M_parent;   
		}
		if (__root == __z)
		{
			__root = __x;
		}
		else 
		{
			if (__z->_M_parent->_M_left == __z)
			{
				__z->_M_parent->_M_left = __x;
			}
			else
			{
				__z->_M_parent->_M_right = __x;
			}
		}
		if (__leftmost == __z) 
		{
			if (__z->_M_right == 0)        // __z->_M_left must be null also
			{
				__leftmost = __z->_M_parent;
			}
			// makes __leftmost == _M_header if __z == __root
			else
			{
				__leftmost = _Rb_tree_node_base::_S_minimum(__x);
			}
		}
		if (__rightmost == __z)  
		{
			if (__z->_M_left == 0)         // __z->_M_right must be null also
			{
				__rightmost = __z->_M_parent;  
			}
			// makes __rightmost == _M_header if __z == __root
			else                      // __x == __z->_M_left
			{
				__rightmost = _Rb_tree_node_base::_S_maximum(__x);
			}
		}
	}
	if (__y->_M_color != _M_red) 
	{ 
		while (__x != __root && (__x == 0 || __x->_M_color == _M_black))
		{
			if (__x == __x_parent->_M_left) 
			{
				_Rb_tree_node_base* __w = __x_parent->_M_right;
				if (__w->_M_color == _M_red) 
				{
					__w->_M_color = _M_black;
					__x_parent->_M_color = _M_red;
					_Rb_tree_rotate_left(__x_parent, __root);
					__w = __x_parent->_M_right;
				}
				if ((__w->_M_left == 0 || 
					__w->_M_left->_M_color == _M_black) &&
					(__w->_M_right == 0 || 
					__w->_M_right->_M_color == _M_black)) 
				{
					__w->_M_color = _M_red;
					__x = __x_parent;
					__x_parent = __x_parent->_M_parent;
				} 
				else 
				{
					if (__w->_M_right == 0 
						|| __w->_M_right->_M_color == _M_black) 
					{
						if (__w->_M_left)
						{
							__w->_M_left->_M_color = _M_black;
						}
						__w->_M_color = _M_red;
						_Rb_tree_rotate_right(__w, __root);
						__w = __x_parent->_M_right;
					}
					__w->_M_color = __x_parent->_M_color;
					__x_parent->_M_color = _M_black;
					if (__w->_M_right) 
					{
						__w->_M_right->_M_color = _M_black;
					}
					_Rb_tree_rotate_left(__x_parent, __root);
					break;
				}
			}
			else 
			{
				// same as above, with _M_right <-> _M_left.
				_Rb_tree_node_base* __w = __x_parent->_M_left;
				if (__w->_M_color == _M_red) 
				{
					__w->_M_color = _M_black;
					__x_parent->_M_color = _M_red;
					_Rb_tree_rotate_right(__x_parent, __root);
					__w = __x_parent->_M_left;
				}
				if ((__w->_M_right == 0 || 
					__w->_M_right->_M_color == _M_black) &&
					(__w->_M_left == 0 || 
					__w->_M_left->_M_color == _M_black)) 
				{
					__w->_M_color = _M_red;
					__x = __x_parent;
					__x_parent = __x_parent->_M_parent;
				} 
				else 
				{
					if (__w->_M_left == 0 || __w->_M_left->_M_color == _M_black) 
					{
						if (__w->_M_right)
						{
							__w->_M_right->_M_color = _M_black;
						}
						__w->_M_color = _M_red;
						_Rb_tree_rotate_left(__w, __root);
						__w = __x_parent->_M_left;
					}
					__w->_M_color = __x_parent->_M_color;
					__x_parent->_M_color = _M_black;
					if (__w->_M_left) 
					{
						__w->_M_left->_M_color = _M_black;
					}
					_Rb_tree_rotate_right(__x_parent, __root);
					break;
				}
			}
		}
		if (__x)
		{
			__x->_M_color = _M_black;
		}
	}
	return __y;
}

//////////////////////////////////////////////////////////////////
// Methods
//////////////////////////////////////////////////////////////////
inline void Rb_tree::erase(_Link_type& __node)
{
	_Link_type __y = (_Link_type) _Rb_tree_rebalance_for_erase(__node,
												_M_header->_M_parent,
												_M_header->_M_left,
												_M_header->_M_right);
	//destroy_node(__y);
	--_M_node_count;
}

inline _Link_type Rb_tree::find(const key_type& __k)
{
	_Link_type __y = _M_header;  // Last node which is not less than __k. 
	_Link_type __x = _M_root();  // Current node. 

	while (__x != 0) 
	{
		//if (!_M_key_compare(__x->GetKey(), __k))
		if (!(__x->_M_value_field._mKey < __k))
		{
			__y = __x, __x = _S_left(__x);
		}
		else
		{
			__x = _S_right(__x);
		}
	}

	//iterator __j = iterator(__y);
	//return (__y == end() || _M_key_compare(__k, __y->GetKey())) ? end() : __y;
	return (__y == end() || (__k < __y->_M_value_field._mKey)) ? end() : __y;
}

inline bool Rb_tree::insert_unique(key_type __key, void* __block, void* __mem)
{
	_Link_type __y = _M_header;
	_Link_type __x = _M_root();
	bool __comp = true;
	while (__x != 0) 
	{
		__y = __x;
		//__comp = _M_key_compare(__key, __x->GetKey());
		__comp = (__key < __x->_M_value_field._mKey);
		__x = __comp ? _S_left(__x) : _S_right(__x);
	}
	//iterator __j = iterator(__y);   
	_Link_type __j = __y;
	if (__comp)
	{
		if (__j == begin())
		{
			//return pair<iterator,bool>(_M_insert(__x, __y, __v), true);
			_M_insert(__x, __y, __key, __block, __mem);
			return true;
		}
		else
		{
			// decrement iterator
			//--__j;
			_M_decrement(__j);
		}
	}
	//if (_M_key_compare(__j->GetKey(), __key))
	if ((__j->_M_value_field._mKey < __key))
	{
		//return pair<iterator,bool>(_M_insert(__x, __y, __v), true);
		_M_insert(__x, __y, __key, __block, __mem);
		return true;
	}
	//return pair<iterator,bool>(__j, false);
	return false;
}



inline void 
_Rb_tree_rebalance(_Rb_tree_node_base* __x, _Rb_tree_node_base*& __root)
{
	__x->_M_color = _M_red;
	while (__x != __root && __x->_M_parent->_M_color == _M_red) 
	{
		if (__x->_M_parent == __x->_M_parent->_M_parent->_M_left) 
		{
			_Rb_tree_node_base* __y = __x->_M_parent->_M_parent->_M_right;
			if (__y && __y->_M_color == _M_red) 
			{
				__x->_M_parent->_M_color = _M_black;
				__y->_M_color = _M_black;
				__x->_M_parent->_M_parent->_M_color = _M_red;
				__x = __x->_M_parent->_M_parent;
			}
			else 
			{
				if (__x == __x->_M_parent->_M_right) 
				{
					__x = __x->_M_parent;
					_Rb_tree_rotate_left(__x, __root);
				}
				__x->_M_parent->_M_color = _M_black;
				__x->_M_parent->_M_parent->_M_color = _M_red;
				_Rb_tree_rotate_right(__x->_M_parent->_M_parent, __root);
			}
		}
		else 
		{
			_Rb_tree_node_base* __y = __x->_M_parent->_M_parent->_M_left;
			if (__y && __y->_M_color == _M_red) 
			{
				__x->_M_parent->_M_color = _M_black;
				__y->_M_color = _M_black;
				__x->_M_parent->_M_parent->_M_color = _M_red;
				__x = __x->_M_parent->_M_parent;
			}
			else 
			{
				if (__x == __x->_M_parent->_M_left) 
				{
					__x = __x->_M_parent;
					_Rb_tree_rotate_right(__x, __root);
				}
				__x->_M_parent->_M_color = _M_black;
				__x->_M_parent->_M_parent->_M_color = _M_red;
				_Rb_tree_rotate_left(__x->_M_parent->_M_parent, __root);
			}
		}
	}
	__root->_M_color = _M_black;
}

inline _Link_type Rb_tree::_M_insert(_Base_ptr __x_, _Base_ptr __y_, key_type __key, void* __block, void* __mem)
{
	_Link_type __x = (_Link_type) __x_;
	_Link_type __y = (_Link_type) __y_;
	_Link_type __z;

	//if (__y == _M_header || __x != 0 || _M_key_compare(__key, __y->GetKey()))
	if (__y == _M_header || __x != 0 || (__key < __y->_M_value_field._mKey)) 
	{
		__z = _M_create_node(__key, __block, __mem);
		_S_left(__y) = __z;               // also makes _M_leftmost() = __z 
		//    when __y == _M_header
		if (__y == _M_header) 
		{
			_M_root() = __z;
			_M_rightmost() = __z;
		}
		else if (__y == _M_leftmost())
		{
			_M_leftmost() = __z; // maintain _M_leftmost() pointing to min node
		}
	}
	else 
	{
		__z = _M_create_node(__key, __block, __mem);
		_S_right(__y) = __z;
		// Maintain _M_rightmost() pointing to max node.
		if (__y == _M_rightmost())
		{
			_M_rightmost() = __z; 
		}
	}
	__z->_M_parent = __y;
	__z->_M_left   = 0;
	__z->_M_right  = 0;
	_Rb_tree_rebalance(__z, _M_header->_M_parent);
	++_M_node_count;

	return __z;
}

#endif // _AT_TREE_H
