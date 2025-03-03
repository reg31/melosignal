# melosignal

This is a c++20 replacement for Qt signal and slots.

There already many similar libraries on Github, and I used some of them, however they did not completely fit my use case:

 - A minimalist code
 - Handle direct and queued connections automatically
 - Easy to modify to suit your own requirement
 - Use seamlessly the Qt way (make it easy and not boilerplate to implement)

The cons I saw with most libraries is that they add hundreds/thousands of line of codes just for me to do a direct connection, and the few libraries also allowing queued connection required to use the observer pattern, making implementation more complex.

I was working on a [translator](https://play.google.com/store/apps/details?id=dands.technologies.melo) and wanted to reduce my dependencies to QObject.

## Features

 - Support function pointers and lamdas
 - Support member functions with different reference types
 - Support connecting one signal to another
 - Automatic direct or queued connection based on thread affinity
 - Bring signal and slots to any object, no need to subclass QObject
 - Minimal footprint, only 100 lines of code

A positive side effect is that this reduces binary side, as you need to subclass QObject less often, for example if you need to use [movetothread()](https://doc.qt.io/qt-6/qobject.html#moveToThread), only the moved object has to subclass QObject, the object(s) connected to its signal do not.

Just add #include "signal.h"and start connecting.

## Examples

### 1️⃣ Creating a Signal
```cpp
#include "signal.h"

melo::signal<int> mySignal;  // Signal that emits an `int`
```

### 2️⃣ Connecting Function Pointers and Lambdas
```cpp
void globalFunction(int value) {
    std::cout << "Global function received: " << value << std::endl;
}

int main() {
    melo::signal<int> mySignal;

    // Connect a function pointer
    mySignal.connect(globalFunction);

    // Connect a lambda
    mySignal.connect([](int value) {
        std::cout << "Lambda received: " << value << std::endl;
    });

    // Emit the signal
    mySignal.emit(42);

    return 0;
}
```
**Output**:

    Global function received: 42
    Lambda received: 42

### 3️⃣ Connecting Member Functions with Different Reference Types
```cpp
class Receiver {
public:
    void onSignal(int value) {
        std::cout << "Member function received: " << value << std::endl;
    }
};

int main() {
    melo::signal<int> mySignal;
    Receiver receiver;

    // Connect member function
    mySignal.connect(&receiver, &Receiver::onSignal);

    // Emit the signal
    mySignal.emit(100);

    return 0;
}
```
**Output**:

    Member function received: 100

### 4️⃣ Connecting One Signal to Another
```cpp
int main() {
    melo::signal<int> signalA;
    melo::signal<int> signalB;

    // Connect signalA to signalB
    signalA.connect(signalB);

    // Connect a lambda to signalB
    signalB.connect([](int value) {
        std::cout << "Signal B received: " << value << std::endl;
    });

    // Emit signalA -> should trigger signalB
    signalA.emit(200);

    return 0;
}
```
**Output**:

    Signal B received: 200

### 5️⃣ Disconnecting signals
```cpp
#include "signal.h"
#include <iostream>

void callbackFunction(int value) {
    std::cout << "Received: " << value << std::endl;
}

int main() {
    melo::signal<int> mySignal;

    // Connect a function
    mySignal.connect(callbackFunction);

    // Emit the signal (should call `callbackFunction`)
    std::cout << "Before disconnect:" << std::endl;
    mySignal.emit(42);

    // Disconnect all slots
    mySignal.disconnect();

    // Emit again (no output expected)
    std::cout << "After disconnect:" << std::endl;
    mySignal.emit(42);

    return 0;
}
```
**Output**:

    Before disconnect:
    Received: 42
    After disconnect:

(After disconnecting, no function is called.)

-   `connect(callbackFunction);` → Connects a function to the signal.
-   `emit(42);` → Calls the function.
-   `disconnect();` → Removes all connected slots.
-   `emit(42);` again → Nothing happens because all connections are removed.

## Limitations

#### c++20 minimum required

    QMetaObject::invokeMethod(slot.thread, [slot, args...]() mutable {
        slot.function(std::forward<Args>(args)...);
    }, Qt::QueuedConnection);

The way this library uses parameter pack expansion is s only valid starting **C++20**.

### How does autoconnect work

When creating a connection, a pointer to the calling/receiver object's thread is saved using QThread::currentThread(), then signal is emited, this pointer is compared to the emiter's thread using QThread:: isCurrentThread().

The code decides whether to use direct or queued execution:
```
if (slot.thread->isCurrentThread()) {
    slot.function(std::forward<Args>(args)...);  // Direct execution
} else {
    QMetaObject::invokeMethod(slot.thread, [slot, args...]() mutable {
        slot.function(std::forward<Args>(args)...);
    }, Qt::QueuedConnection);  // Queued execution
}
```
Breakdown of the Decision Process:

 - If the slot's QThread matches the current thread → Direct Connection
	The function is executed immediately.
 - If the slot's QThread is different from the current thread → Queued  Connection
The function is queued and executed in the target thread’s event loop.

This approach ensures thread safety, preventing function calls from executing in the wrong thread.


### Thread safety

The library is thread safe, connect and emit functions are protected by a QMutex and a QMutexLocker. The mutex is unlikely to be triggered unless you use the library in high performance application.

Qmutex is quite cheap in our use case [anyway](https://doc.qt.io/qt-6/qmutex.html#details):

> QMutex is optimized to be fast in the non-contended case. It will not allocate memory if there is no contention on that mutex. It is constructed and destroyed with almost no overhead, which means it is fine to have many mutexes as part of other classes.

