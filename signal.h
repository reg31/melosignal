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
    using function_ptr = std::function<void(Args...)>;

    struct Slot {
       function_ptr function = nullptr;
       QPointer<QThread> thread = QThread::currentThread();
    };

    std::vector<Slot> slots{};
    QMutex mutex;
	
	inline void insert(const function_ptr &func)
	{
		QMutexLocker locker(&mutex);
		slots.emplace_back(func);
	}

public:

    ~signal() = default;
    signal() { slots.reserve(3); };

    // ✅ Support function pointers and lamdas
	template <typename Func>
	void connect(Func&& func)
	{	
		insert(func);
	}

    // ✅ Support member functions with different reference types
    template <typename ClassType, typename Func>
    void connect(ClassType* instance, Func&& func)
	{		
        insert (
            [instance, func](Args&&... args) {
				std::invoke(func, instance, std::forward<Args>(args)...);
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
            if(slot.function != nullptr && slot.thread)
            {
                if (slot.thread->isCurrentThread()) {
                    slot.function(args...);
                } else {
                    QMetaObject::invokeMethod(slot.thread, [slot, args...] {
                        slot.function(args...);
                    }, Qt::QueuedConnection);
                }
            }
        }
    }
};

} // namespace melo

#endif // SIGNAL_H
