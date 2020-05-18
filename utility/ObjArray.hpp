#ifndef _OBJECT_ARRAY_H_
#define _OBJECT_ARRAY_H_
#include <climits>
/*
* 对象分配管理
* 容器：数组
* 算法：第0个永远为新的元素，元素间用next串联起来，该数组只增长
		插入、删除、访问的算法复杂度都是O1
*/

#include <malloc.h>

struct default_alloctor
{
	static void* malloc(size_t l)
	{
		return ::malloc(l);
	}

	static void* realloc(void* p, size_t l)
	{
		return ::realloc(p, l);
	}

	static void free(void* p)
	{
		::free(p);
	}
};

template<typename T, 
		typename Alloctor = default_alloctor,
		unsigned int INIT_SIZE = 32,
		unsigned int GROW_SIZE = 32>
class ObjArray
{
	struct VAL
	{
		size_t	next;
		T*		element;
	};

	struct construct_default
	{
		void operator()(void* ptr) { ::new (ptr) T(); }
	};

	template<typename P> struct construct_1 
	{
		P& m_p;
		construct_1<P>(P& p):m_p(p){}
		void operator()(void* ptr) { ::new (ptr) T(m_p); }
	};

	template<typename P1, typename P2> struct construct_2
	{
		P1& m_p1; P2& m_p2;
		construct_2<P1,P2>(P1& p1, P2& p2):m_p1(p1),m_p2(p2){}
		void operator()(void* ptr) { ::new (ptr) T(m_p1, m_p2); }
	};

public:
	enum {INVALID_INDEX = 0, USED_INDEX = 0xffffffff};

	ObjArray()
	{
		m_data = NULL;
		m_count = 0;
		m_max_count = UINT_MAX;
		_resize(INIT_SIZE+1);
	}

	~ObjArray()
	{
		if (m_data) {
			for (size_t n = 0; n < m_count; ++n) {
				DelVal(n);
				Alloctor::free(_at(n).element);
			}
			Alloctor::free(m_data);
			m_data = NULL;
		}
	}

protected:
	size_t _size() const 
	{
		return m_count;
	}

	VAL& _at(size_t pos) const
	{
		assert(m_data);
		assert(pos < m_count);
		if (!m_data) throw std::runtime_error("invalid m_data in ObjArray");
		if (pos >= m_count) throw std::runtime_error("invalid pos in ObjArray");
		return m_data[pos];
	}

	void _resize(size_t newSize)
	{
		if (newSize > m_max_count)
			newSize = m_max_count;

		const size_t oldSize = _size();
		if (oldSize >= newSize) 
			return;

		void* data_new_ptr;
		if (m_data)
			data_new_ptr = Alloctor::realloc(m_data, (newSize) * sizeof(VAL));
		else
			data_new_ptr = Alloctor::malloc((newSize) * sizeof(VAL));
		if (!data_new_ptr)
			return;

		m_count = newSize;
		m_data = (VAL*)data_new_ptr;

		size_t newIndex = oldSize;

		if (oldSize > 0) {
			size_t tail = 0, next;
			while ( (next = _at(tail).next) != INVALID_INDEX)
				tail = next;
			_at(tail).next = newIndex;
		}

		for (; newIndex < newSize - 1; ++newIndex) {
			void *ptr = Alloctor::malloc(sizeof(T));
			if (!ptr) break;
			_at(newIndex).element = (T*)ptr;
			_at(newIndex).next = newIndex + 1;
		}

		while (newIndex < newSize) {
			void *ptr = Alloctor::malloc(sizeof(T));
			_at(newIndex).element = (T*)ptr;

			if (!ptr) break;
			_at(newIndex).next = INVALID_INDEX;
			++newIndex;
		}

	}

	/*
	* 存储值，返回值的索引
	*/
	template<typename F>
	T* _new(size_t& id, F& fnConstruct)
	{
		id = _at(0).next;
		if (INVALID_INDEX == id)
		{
			_resize(_size() + GROW_SIZE);
			id = _at(0).next;
		}

		if (id != INVALID_INDEX)
		{
			assert(id != USED_INDEX);
			VAL& val = _at(id);
			if (!val.element) return NULL;

			_at(0).next = val.next;
			val.next = USED_INDEX;
			try { fnConstruct(val.element); }
			catch (...) { DelVal(id); return NULL; }
			return val.element;
		}
		return NULL;
	}

public:
	
	void SetMaxSize(size_t nSize)
	{
		m_max_count = nSize+1;
		if (m_max_count < m_count)
			m_max_count = m_count;
	}

	T* NewVal(size_t& id)
	{
		construct_default c0;
		return _new(id, c0);
	}

	template<typename P>
	T* NewVal(size_t& id, P& p)
	{
		construct_1<P> c1(p);
		return _new(id, c1);
	}

	template<typename P1, typename P2>
	T* NewVal(size_t& id, P1& p1, P2& p2)
	{
		construct_2<P1, P2> c2(p1, p2);
		return _new(id, c2);
	}

	/*
	* 获取值
	*/
	T* GetVal(size_t nValueIndex)
	{
		if (nValueIndex >= _size() ||
			INVALID_INDEX == nValueIndex)
			return NULL;
		VAL& val = _at(nValueIndex);
		if (USED_INDEX == val.next)
			return val.element;
		return NULL;
	}

	/*
	* 回收值
	*/
	void DelVal(size_t nValueIndex)
	{
		if (nValueIndex >= _size())
			return;
		VAL& val = _at(nValueIndex);
		if (USED_INDEX == val.next)
		{
			VAL& first = _at(0);
			val.next = first.next;
			first.next = nValueIndex;
			val.element->~T();
		}
	}

	size_t GetMaxSize()
	{
		return _at();
	}

protected:
	VAL*	m_data;
	size_t	m_count;
	size_t  m_max_count;
};


#endif
