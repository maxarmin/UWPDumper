#include "DumperIPC.hpp"

#include <array>
#include <atomic>
#include <thread>

/// API
namespace IPC
{
/// IPC Message Queue

MessageEntry::MessageEntry()
{
}

MessageEntry::MessageEntry(const wchar_t* String)
{
	wcscpy_s(this->String, MessageEntry::StringSize, String);
}

template<typename QueueType, std::size_t PoolSize>
class AtomicQueue
{
public:
	using Type = QueueType;
	static constexpr std::size_t MaxSize = PoolSize;
	AtomicQueue()
		:
		Head(0),
		Tail(0)
	{
	}
	~AtomicQueue()
	{
	}
	void Enqueue(const Type& Entry)
	{
		while( Mutex.test_and_set(std::memory_order_acquire) ) {}
		Entries[Tail] = Entry;
		Tail = (Tail + 1) % MaxSize;
		Mutex.clear(std::memory_order_release);
	}

	Type Dequeue()
	{
		Type Temp;
		while( Mutex.test_and_set(std::memory_order_acquire) ) {}
		Temp = Entries[Head];
		Head = (Head + 1) % MaxSize;
		Mutex.clear(std::memory_order_release);
		return Temp;
	}
	std::size_t Size()
	{
		while( Mutex.test_and_set(std::memory_order_acquire) ) {}
		Mutex.clear(std::memory_order_release);
		std::size_t Result = Tail - Head;
		return Result;
	}

	bool Empty()
	{
		return Size() == 0;
	}
private:
	std::array<Type, MaxSize> Entries = { Type() };
	std::size_t Head = 0;
	std::size_t Tail = 0;
	std::atomic_flag Mutex = ATOMIC_FLAG_INIT;
};

#pragma data_seg("SHARED")
AtomicQueue<MessageEntry, 1024> MessagePool = {};
std::atomic<std::size_t> CurMessageCount = 0;

// The process we are sending our data to
std::atomic<std::uint32_t> ClientProcess(0);

// The target UWP process we wish to dump
std::atomic<std::uint32_t> TargetProcess(0);

#pragma data_seg()
#pragma comment(linker, "/section:SHARED,RWS")
///

void SetClientProcess(std::uint32_t ProcessID)
{
	ClientProcess = ProcessID;
}

std::uint32_t GetClientProcess()
{
	return ClientProcess;
}

void SetTargetProcess(std::uint32_t ProcessID)
{
	TargetProcess = ProcessID;
}

std::uint32_t GetTargetProcess()
{
	return TargetProcess;
}

void PushMessage(const wchar_t* Message)
{
	PushMessage(MessageEntry(Message));
}

void PushMessage(const MessageEntry& Message)
{
	MessagePool.Enqueue(Message);
}

MessageEntry PopMessage()
{
	return MessagePool.Dequeue();
}

std::size_t MessageCount()
{
	return MessagePool.Size();
}
}