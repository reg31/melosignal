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

template <typename... Args>
class signal
{
private:
    using Callback = std::function<void(Args...)>;

    struct Slot {
        Callback callback = nullptr;
        QPointer<QThread> thread = QThread::currentThread();
    };

    std::vector<Slot> slots{};
    QMutex mutex;

    inline void insert(const Callback &callee)
    {
        QMutexLocker locker(&mutex);
        slots.emplace_back(callee);
    }

public:

    ~signal() = default;
    signal() { slots.reserve(3); };

    // ✅ Support function pointers and lamdas
    template <typename Function>
    void connect(Function&& callee)
    {
        insert(callee);
    }

    // ✅ Support member functions with different reference types
    template <typename ClassType, typename Function>
    void connect(ClassType* instance, Function&& member_function)
    {
        insert (
            [instance, member_function](Args&&... args) {
                std::invoke(member_function, instance, std::forward<Args>(args)...);
            }
        );
    }

    // ✅ Support connecting one signal to another
    template <typename OtherSignal>
    void connect(OtherSignal &other)
    {
        insert ([&other](Args&&... args) { other.emit(std::forward<Args>(args)...); });
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
            if(slot.callback != nullptr)
            {
                if (slot.thread->isCurrentThread()) {
                    slot.callback(args...);
                } else {
                    QMetaObject::invokeMethod(slot.thread, [&cb = slot.callback, args...] {
                        cb(args...);
                    }, Qt::QueuedConnection);
                }
            }
        }
    }
};

} // namespace melo

#endif // SIGNAL_H
