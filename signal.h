#ifndef SIGNAL_H
#define SIGNAL_H

#include <vector>
#include <QThread>
#include <QPointer>
#include <functional>
#include <QMetaObject>
#include <QReadWriteLock>

namespace melo {

template <typename... Args>
class signal
{
private:
    using Callback = std::function<void(Args...)>;

    struct Slot {
        Callback callback;
        QPointer<QThread> thread;
    };

    std::vector<Slot> slots{};
    QReadWriteLock lock;

    inline void insert(Callback&& callee)
    {
        QWriteLocker locker(&lock);
        slots.emplace_back(Slot{std::move(callee), QThread::currentThread()});
    }

public:
    ~signal() = default;
    signal() noexcept = default;

    // ✅ Support function pointers and lamdas
    template <typename Function>
    void connect(Function&& callee)
    {
        insert(std::move(callee));
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
        insert([&other](Args&&... args) { other.emit(std::forward<Args>(args)...); });
    }

    void disconnect()
    {
        QWriteLocker locker(&lock);
        slots.clear();
    }

    void emit(Args... args)
    {
        QReadLocker locker(&lock);

        for (const Slot &slot : slots)
        {
            if(slot.callback != nullptr)
            {
                if (slot.thread->isCurrentThread()) {
                    slot.callback(args...);
                } else {
                    QMetaObject::invokeMethod(slot.thread, [&cb = slot.callback, args...] {cb(args...);}, Qt::QueuedConnection);
                }
            }
        }
    }
};

} // namespace melo

#endif // SIGNAL_H
