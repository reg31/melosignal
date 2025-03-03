#ifndef SIGNAL_H
#define SIGNAL_H

#include <QMutex>
#include <vector>
#include <QThread>
#include <QPointer>
#include <functional>
#include <QMetaObject>
#include <QMutexLocker>


namespace melo {

// Templated Signal class
template <typename... Args>
class signal
{
private:
    using function_ptr = std::function<void(Args...)>;

    struct Slot {
       function_ptr function = nullptr;
        QPointer<QThread> thread = nullptr;
    };

    std::vector<Slot> slots{};
    QMutex mutex;

public:

    ~signal() = default;
    signal() { slots.reserve(3); };

    // ✅ Support function pointers and lamdas
    void connect(function_ptr slot)
    {
        QMutexLocker locker(&mutex);
        slots.emplace_back(std::move(slot), QThread::currentThread());
    }

    // ✅ Support member functions with different reference types
    template <typename ClassType, typename... SlotArgs>
    void connect(ClassType* instance, void (ClassType::*member_func)(SlotArgs...))
    {
        QMutexLocker locker(&mutex);

        slots.emplace_back (
            [instance, member_func](Args... value) {
                (instance->*member_func)(std::forward<decltype(args)>(args)...);
            },
            QThread::currentThread()
        );
    }

    // ✅ Support connecting one signal to another
    void connect(signal<Args...>& other)
    {
        QMutexLocker locker(&mutex);

        slots.emplace_back (
            [&other](Args... args) { other.emit(args...); },
            QThread::currentThread()
        );
    }

    void disconnect()
    {
        QMutexLocker locker(&mutex);
        slots.clear();
    }

    void emit(Args... args)
    {
        QMutexLocker locker(&mutex);

        for (const Slot &slot : slots)
        {
            if(slot.function != nullptr && slot.thread)
            {
                if (slot.thread->isCurrentThread()) {
                    slot.function(std::forward<Args>(args)...);
                } else {
                    QMetaObject::invokeMethod(slot.thread, [slot, args...]() mutable {
                        slot.function(std::forward<Args>(args)...);
                    }, Qt::QueuedConnection);
                }
            }
        }
    }
};

} // namespace melo

#endif // SIGNAL_H
