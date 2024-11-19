#include <exception>

template<class T> class QueueList {
public:
    struct Element {
        Element *next;
        T value;
    };
    
    Element *head = nullptr, *tail = nullptr;
    
    void push(T v) {
        Element *el = new Element{nullptr, v};
        if (!head) {
            head = el;
            tail = el;
            return;
        }
        tail->next = el;
        tail = el;
    }
    
    T pop() {
        if (!head) throw std::length_error("Empty list!");
        if (head == tail) {
            T v = head->value;
            delete head;
            head = nullptr;
            tail = nullptr;
            return v;
        }
        Element e = *head;
        delete head;
        head = e.next;
        return e.value;
    }
};

template<typename ...Args> class Event {
public:
    using Listener = void (*)(Args...);
    
    QueueList<Listener> listeners;
    
    typedef typename QueueList<Listener>::Element Element;
    
    void subscribe(Listener listener) {
        listeners.push(listener);
    }
    
    void unsubscribe(Listener listener) {
        if (!listeners.head) {return;}
        if (listeners.head->value == listener) {
            listeners.pop();
        }
        Element *el = listeners.head->next;
        Element *last = listeners.head;
        while (el != nullptr) {
            if (el->value == listener) {
                last->next = el->next;
                delete el;
            }
            last = el;
            el = el->next;
        }
    }
    
    void call(Args... args) const {
        if (!listeners.head) {return;}
        Element *el = listeners.head;
        while (el != nullptr) {
            el->value(args...);
            el = el->next;
        }
    }
};

// Only triggers events until one "handles" it. Do not do anything if you don't handle it.
template<typename ...Args> class EventUnhandled {
public:
    using Listener = bool (*)(Args...);
    
    QueueList<Listener> listeners;
    
    typedef typename QueueList<Listener>::Element Element;
    
    void subscribe(Listener listener) {
        listeners.push(listener);
    }
    
    void unsubscribe(Listener listener) {
        if (!listeners.head) {return;}
        if (listeners.head->value == listener) {
            listeners.pop();
        }
        Element *el = listeners.head->next;
        Element *last = listeners.head;
        while (el != nullptr) {
            if (el->value == listener) {
                last->next = el->next;
                delete el;
            }
            last = el;
            el = el->next;
        }
    }
    
    bool call(Args... args) const {
        if (!listeners.head) {return false;}
        Element *el = listeners.head;
        while (el != nullptr) {
            if (el->value(args...)) return true;
            el = el->next;
        }
        return false;
    }
};