// Implements a queue through a linked list, but exposes the pointers for your leisure
template<class T> class QueueList {
public:
    ~QueueList() {
        Element current = head;
        while (current) {
            Element next = current->next;
            delete current;
            current = next;
        }
    }
    
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

// Allows you to call multiple function pointers at a time
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

/* Allows you to call function pointers until one handles the event. The order added is the order called.
 * Do not subscribe functions that could/will modify inputs or perform external actions yet not handle the event.
*/
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
