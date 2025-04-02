#ifndef SIGNAL_H
#define SIGNAL_H

#include <vector>
#include <QThread>
#include <QPointer>
#include <QObject>
#include <functional>
#include <QMetaObject>
#include <QReadWriteLock>
#include <type_traits>

namespace melo {

template <typename... Args>
class signal
{
private:
    using Callback = std::function<void(Args...)>;

    struct Slot {
        Callback callback;
        QPointer<QObject> qobject;
    };

    std::vector<Slot> slots{};
    QReadWriteLock lock;

    inline void insert(Callback&& callee, QPointer<QObject> obj = nullptr)
    {
        QWriteLocker locker(&lock);
        slots.emplace_back(Slot{std::move(callee), obj? obj : QThread::currentThread()});
    }

public:
    ~signal() = default;
    signal() noexcept = default;

    // Support function pointers and lamdas
    template <typename Function>
	requires std::invocable<Function, Args...>
    void connect(Function&& callee)
    {
        insert(std::move(callee));
    }

    // Support member functions with different reference types
    template <typename ClassType, typename Function>
	requires std::invocable<Function, ClassType*, Args...>
    void connect(ClassType* instance, Function&& member_function)
    {
		QPointer<QObject> obj = nullptr;
		
        if constexpr (std::is_base_of_v<QObject, ClassType>) {
            obj = instance;
		}
		
		insert([instance, member_function](Args&&... args) {
			std::invoke(member_function, instance, std::forward<Args>(args)...);
		}, obj);
    }

    // Support connecting one signal to another
    template <typename OtherSignal>
	requires std::same_as<OtherSignal, signal<Args...>>
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
            if(slot.callback && slot.qobject)
            {
				QMetaObject::invokeMethod(
					slot.qobject, 
					[cb = slot.callback, ...args = std::move(args)] { cb(args...); },
					Qt::AutoConnection
				);			
            }
        }
    }
};

} // namespace melo

#endif // SIGNAL_H
