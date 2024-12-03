#include <exception>

namespace WellSpring {
  // Contains helper classes for Callable, as well as Callable itself
  namespace callable {
    
    enum class _FuncType {
      NONE = 0,
      FUNC,
      METHOD,
    };

    template<class Ret, class... Args> class _FunctionBase {
    public:
      virtual _FuncType get_type() const { return _FuncType::NONE; }
      virtual Ret call(Args...) const = 0;
      virtual _FunctionBase *clone() const = 0;
    };
    
    template<class Ret, class... Args> class _FunctionFunc : public _FunctionBase<Ret, Args...> {
    protected:
      typedef Ret (*FPtr)(Args...);
      FPtr func; 
    public:
      int refcount = 0;
      _FuncType get_type() const override { return _FuncType::FUNC; }
      Ret call(Args... args) const override { return func(args...); }
      
      _FunctionBase<Ret, Args...> *clone() const override {
        return new _FunctionFunc(*this);
      }
      
      _FunctionFunc(FPtr f) : func(f) {}
      _FunctionFunc(const _FunctionFunc &other) : func(other.func) {}
    };
    
    template<class T, class Ret, class... Args> class _FunctionMethod : public _FunctionBase<Ret, Args...> {
    protected:
      T *data;
      
      typedef Ret (T::*FPtr)(Args...);
      FPtr func; 
    public:
      _FuncType get_type() const override { return _FuncType::METHOD; }
      Ret call(Args... args) const override { 
        // Should we do it? Yes, but it isn't our job.
        //if (data == nullptr) throw std::exception();
        return (data->*func)(args...);
      }
      
      _FunctionBase<Ret, Args...> *clone() const override {
        return new _FunctionMethod(*this);
      }
      
      _FunctionMethod(FPtr method, T *obj) : data(obj), func(method) {}
      _FunctionMethod(const _FunctionMethod &other) : data(other.data), func(other.func) {}
    };
    
    template<class T> class Callable;
    
    template<class Ret, class... Args> class Callable<Ret(Args...)> {
    protected:
      _FunctionBase<Ret, Args...> *func;
    public:
      bool is_valid() const {
        if (!func) return false;
        if (func->get_type() == _FuncType::NONE) return false;
        return true;
      }
      
      Ret operator()(Args... args) const {
        // Better to warn them first
        if (!is_valid()) throw std::exception();
        return func->call(args...);
      }

      Callable &operator=(const Callable &) {
        if (other.is_valid()) {
          func = other.func->clone();
          func->refcount++;
        } else {
          throw std::invalid_argument("New Callable");
        }
      }
      
      template<class T> Callable(Ret (T::*method)(Args...), T *obj) :
        func(new _FunctionMethod<T, Ret, Args...>(method, obj))
      { func->refcount++; }
      
      Callable(Ret (*f)(Args...)) : func(new _FunctionFunc<Ret, Args...>(f)) { func->refcount++; }
      Callable() : func(nullptr) {}
      Callable(const Callable &other) {
        if (other.is_valid()) {
          func = other.func->clone();
          func->refcount++;
        }
      }
      
      ~Callable() {
        if (func) {
          func->refcount--;
          // You never know...
          if (func->refcount <= 0)
            delete func;
        }
      }
    };
  }
}

// Export Callable
using WellSpring::callable::Callable;

// NOTE: Use this in a Callable constructor to make a Callable to an instance member function, especially an anonymous one
#define MEMBER_FUNC(instance, func) &decltype(instance)::func, &instance
