#include <stdexcept>
#include <cstring>

template<class T> class RoundQueue {
private:
  int _used = 0, _size = 0, _start = 0;
  T *_data = nullptr;
  int _index_to_buffer(int index) const {
    return (index + _start) % _size;
  }
public:
  RoundQueue(int initial_size) : _size(initial_size), _data(new T[initial_size]) {}
  RoundQueue() {}
  RoundQueue(const RoundQueue &other) :
    _used(other._used), _start(other._start), _size(other._size), _data(new T[other._size])
  {
    memcpy(_data, other._data, _size * sizeof(T));
  }
  
  ~RoundQueue() {
    delete[] _data;
  }
  
  // Elaborate due to the possibility of x = x, in which case we use deleted memory unless we
  //  do this
  RoundQueue &operator=(const RoundQueue &other) {
    int used = other._used;
    int size = other._size;
    int start = other._start;
    T *data = new T[_size];
    memcpy(data, other._data, size * sizeof(T));
    if(_data)
      delete[] _data;
    _used = used;
    _size = size;
    _start = start;
    _data = data;
  }
  
  T &peek() {
    if (_size == 0) throw std::runtime_error("Peeking with nothing to peek!");
    return _data[_index_to_buffer(0)];
  }

  bool operator==(const RoundQueue &other) const {
    if (_data == nullptr && other._data == nullptr) return true;
    if (_used != other._used) return false;
    for (int i = 0; i < _used; ++i) {
      if (_data[_index_to_buffer(i)] != other._data[_index_to_buffer(i)]) return false;
    }
  }

  bool operator!=(const RoundQueue &other) const {
    return !(*this == other);
  }

  void rebuild(int more) {
    if (more < 0) throw std::length_error("More < 0");
    T *data = new T[_size + more];
    if (_data) {
      for (int i = 0; i < _used; ++i) {
        data[i] = _data[_index_to_buffer(i)];
      }
      delete[] _data;
    }
    _start = 0;
    _data = data;
    _size += more;
  }
  
  void push(T value) {
    _used++;
    if (_used > _size) rebuild(_size == 0 ? 1 : _size);
    _data[_index_to_buffer(_used - 1)] = value;
  }
  
  T pop() {
    if (_used == 0) throw std::runtime_error("Popping with nothing to pop!");
    T value = _data[_start];
    _used--;
    if (_used == 0) _start = 0;
    else _start = (_start + 1) % _size;
    return value;
  }
};
