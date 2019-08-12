#pragma once
#pragma once
#include <mutex>
#include <assert.h>

/*
*
*
* 结构模型：
left_bottom_unit           left_first_unit  right_first_unit       right_top_unit
*                          _____________________________________________________
*                         |                            |                        |
*                         |____________________________|________________________|
* 方法：
*           push_left push_stack                      read                     push_right
*           get_left                                  read_back                get_right
*           destroy_left                                                       destroy_right
*           clear_left                                                         clear_right
* 	        get_left_count                                                     get_right_count
*
*
*/


const int RIGHT_COUNT_OVERDUAL = 5;
const int LEFT_COUNT_OVERDUAL = 5;


template<typename T>
class PlayBufferUnit
{
public:
	PlayBufferUnit(T *data);
	~PlayBufferUnit();

	PlayBufferUnit<T> *right_unit;
	PlayBufferUnit<T> *left_unit;
	T *data;
};

template<typename T>
class PlayBuffer
{
public:
	PlayBuffer();

	bool push_right(T *data);
	bool push_left();
	bool push_stack(T *data);
	T *read();
	T *read_back();
	T *get_right();
	T *get_left();
	bool destroy_right();
	bool destroy_left();
	bool clear_right();
	bool clear_left();
	int get_right_count();
	int get_left_count();

private:
	//PlayBufferUnit<T> *current_frame;
	PlayBufferUnit<T> *right_top_unit;
	PlayBufferUnit<T> *right_first_unit;
	PlayBufferUnit<T> *left_bottom_unit;
	PlayBufferUnit<T> *left_first_unit;

	//frame_stack基于栈模型，代表当前栈顶对象
	PlayBufferUnit<T> *frame_stack;

	//buffer的right区域数量
	int right_count;
	//buffer的left区域数量
	int left_count;

	std::mutex buffer_mt;
};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
PlayBufferUnit<T>::PlayBufferUnit(T *data) :right_unit(NULL), left_unit(NULL)
{
	this->data = data;
}

template<typename T>
PlayBufferUnit<T>::~PlayBufferUnit()
{
	delete data;
}

//////////////////////////////////////////////////////////////////////////////////

template<typename T>
PlayBuffer<T>::PlayBuffer() :right_top_unit(NULL), right_first_unit(NULL), left_bottom_unit(NULL),
left_first_unit(NULL), frame_stack(NULL), right_count(0), left_count(0)
{
}

template<typename T>
bool PlayBuffer<T>::push_right(T *data)
{
	std::lock_guard<std::mutex> lgd(buffer_mt);
	PlayBufferUnit<T> *buff = new PlayBufferUnit<T>(data);
	if (this->right_top_unit != NULL)
	{
		//说明right单元列表不为空
		this->right_top_unit->right_unit = buff;
		buff->left_unit = this->right_top_unit;
		this->right_top_unit = buff;
	}
	else if (this->left_bottom_unit != NULL)
	{
		//说明left单元列表不为空
		this->left_first_unit->right_unit = buff;
		buff->left_unit = this->left_first_unit;
		this->right_first_unit = buff;
		this->right_top_unit = buff;
	}
	else
	{
		//说明buffer为空
		this->right_first_unit = buff;
		this->right_top_unit = buff;
	}
	right_count++;

	return true;
}

/*
* 将frame_stack栈的依次push到left bottom
*/
template<typename T>
bool PlayBuffer<T>::push_left()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	//PlayBufferUnit<T> left_bottom = this->left_bottom_unit;
	if (this->frame_stack != NULL)
	{
		PlayBufferUnit<T> *tmp = this->frame_stack;

		while (tmp != NULL)
		{
			if (this->left_bottom_unit != NULL)
			{
				this->left_bottom_unit->left_unit = tmp;
				tmp->right_unit = this->left_bottom_unit;
			}
			else
			{
				this->left_first_unit = tmp;
				if (this->right_first_unit != NULL)
				{
					this->left_first_unit->right_unit = this->right_first_unit;
					this->right_first_unit->left_unit = this->left_first_unit;
				}
			}
			this->left_bottom_unit = tmp;
			tmp = tmp->left_unit;
			left_count++;
		}
		this->frame_stack = NULL;
	}

	return true;
}

template<typename T>
bool PlayBuffer<T>::push_stack(T *data)
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	PlayBufferUnit<T> *buff = new PlayBufferUnit<T>(data);
	if (this->frame_stack != NULL)
	{
		buff->left_unit = this->frame_stack;
		this->frame_stack = buff;
	}
	else
	{
		this->frame_stack = buff;
	}

	return true;
}

template<typename T>
T *PlayBuffer<T>::read()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	if (this->right_first_unit != NULL)
	{
		T *res = this->right_first_unit->data;
		if (this->left_first_unit == NULL)
		{
			this->left_first_unit = this->right_first_unit;
			this->left_bottom_unit = this->right_first_unit;
		}
		else
		{
			this->left_first_unit = this->right_first_unit;
		}

		this->right_first_unit = this->right_first_unit->right_unit;
		if (this->right_first_unit == NULL)
		{
			this->right_top_unit = NULL;
		}
		right_count--;
		left_count++;
		return res;
	}
	else {
		return NULL;
	}

}

template<typename T>
T *PlayBuffer<T>::read_back()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	if (this->left_first_unit != NULL)
	{
		T *res = this->left_first_unit->data;
		if (this->right_first_unit == NULL)
		{
			this->right_first_unit = this->left_first_unit;
			this->right_top_unit = this->left_first_unit;
		}
		else
		{
			this->right_first_unit = this->left_first_unit;
		}

		this->left_first_unit = this->left_first_unit->left_unit;
		if (this->left_first_unit == NULL)
		{
			this->left_bottom_unit = NULL;
		}
		right_count++;
		left_count--;
		return res;
	}
	else
	{
		return NULL;
	}

}

template<typename T>
T *PlayBuffer<T>::get_right()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	if (this->right_top_unit != NULL)
	{
		return this->right_top_unit->data;
	}
	else
	{
		return NULL;
	}
}

template<typename T>
T *PlayBuffer<T>::get_left()
{
	if (this->left_bottom_unit != NULL)
	{
		return this->left_bottom_unit->data;
	}
	else
	{
		return NULL;
	}
}

template<typename T>
bool PlayBuffer<T>::destroy_right()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	while (this->right_count > RIGHT_COUNT_OVERDUAL)
	{
		PlayBufferUnit<T> *buff = this->right_top_unit;
		this->right_top_unit = this->right_top_unit->left_unit;
		this->right_top_unit->right_unit = NULL;
		delete buff;
		this->right_count--;
	}

	return true;
}

template<typename T>
bool PlayBuffer<T>::destroy_left()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	while (this->left_count > LEFT_COUNT_OVERDUAL)
	{
		PlayBufferUnit<T> *buff = this->left_bottom_unit;
		this->left_bottom_unit = this->left_bottom_unit->right_unit;
		this->left_bottom_unit->left_unit = NULL;
		delete buff;
		this->left_count--;
	}

	return true;
}

template<typename T>
bool PlayBuffer<T>::clear_right()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	PlayBufferUnit<T> *buff = NULL;
	while (this->right_top_unit != NULL)
	{
		buff = this->right_top_unit;
		//bool breakFlag = false;
		if (this->right_top_unit == this->right_first_unit)
		{
			this->right_top_unit = NULL;
			this->right_first_unit = NULL;
			if (this->left_first_unit != NULL)
			{
				this->left_first_unit->right_unit = NULL;
			}
			//breakFlag = true;
		}
		else
		{
			this->right_top_unit = this->right_top_unit->left_unit;
			this->right_top_unit->right_unit = NULL;
		}

		delete buff;
		this->right_count--;

		//if (breakFlag)
		//{
		//	break;
		//}
	}
	assert(this->right_count >= 0);

	return true;
}

template<typename T>
bool PlayBuffer<T>::clear_left()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);

	PlayBufferUnit<T> *buff = NULL;
	while (this->left_bottom_unit != NULL)
	{
		buff = this->left_bottom_unit;
		//bool breakFlag = false;
		if (this->left_bottom_unit == this->left_first_unit)
		{
			this->left_bottom_unit = NULL;
			this->left_first_unit = NULL;
			if (this->right_first_unit != NULL)
			{
				this->right_first_unit->left_unit = NULL;
			}
			//breakFlag = true;
		}
		else
		{
			this->left_bottom_unit = this->left_bottom_unit->right_unit;
			this->left_bottom_unit->left_unit = NULL;
		}

		delete buff;
		this->left_count--;

		//if (breakFlag)
		//{
		//	break;
		//}
	}

	assert(this->left_count >= 0);

	return true;
}

template<typename T>
int PlayBuffer<T>::get_right_count()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);
	return this->right_count;
}

template<typename T>
int PlayBuffer<T>::get_left_count()
{
	std::lock_guard<std::mutex> lgd(buffer_mt);
	return this->left_count;
}
