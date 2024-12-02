#include <iostream>
#include <functional>
#include <unordered_map>
#include <exception>

template<class Ret, class... Args> class FunctionRegistry;

template<class Ret, class... Args>
class Function {
private:
    int value;
    friend class FunctionRegistry<Ret, Args...>;
    
    Function() : value(0) {}
    
    static constexpr Function from_int(int x) {
      Function func;
      func.value = x;
      return func;
    }
    
    int to_int() const {
      return value;
    }
public:
    // Deleted conversions to prevent implicit casting
    template<class R, class... A>
    Function(const Function<R, A...>&) = delete;

    bool operator==(const Function& other) const { return value == other.value; }
    bool operator!=(const Function& other) const { return value != other.value; }
};

template<class Ret, class... Args> class FunctionRegistry {
public:
    using FuncType = std::function<Ret(Args...)>;
    using FuncIndex = Function<Ret, Args...>;
private:
    static std::unordered_map<int, FuncType> funcs;
    static int current_index;
public:
    static Ret call(FuncIndex function, Args... args) {
        int index = function.to_int();
        if (funcs.count(index) < 0) throw std::bad_function_call();
        return funcs.at(index)(args...);
    }
    
    template<class T> static FuncIndex add(const T &func) {
        return add(FuncType(func));
    }
    
    static FuncIndex add(const FuncType &func) {
        funcs[++current_index] = func;
        return Function<Ret, Args...>::from_int(current_index);
    }
};

template<class Ret, class... Args>
std::unordered_map<int, std::function<Ret(Args...)>>
FunctionRegistry<Ret, Args...>::funcs = std::unordered_map<int, std::function<Ret(Args...)>>();

template<class Ret, class... Args>
int
FunctionRegistry<Ret, Args...>::current_index = 0;

int main() {
    Function<void> func = FunctionRegistry<void>::add([&](){
      std::cout << "Ooga Booga\n";
    });
    
    FunctionRegistry<void>::call(func);

    return 0;
}
