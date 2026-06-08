#pragma once

#include <Arduino_FreeRTOS.h>
#include <queue.h>

template<typename T, int N>
struct Queue{
	QueueHandle_t handle;

	// Queue() = default;
	
	Queue(){
		handle = xQueueCreate(N, sizeof(T));
	}

	void send(const T& item){
		xQueueSend(handle, &item, portMAX_DELAY);
	}

	T receive(){
		T item;
		xQueueReceive(handle, &item, portMAX_DELAY);
		return item;
	}

	bool receiveUntil(T& out, TickType_t tickValue){
		return xQueueReceive(handle, &out, tickValue) == pdTRUE;
	}

	void clear(){
		xQueueReset(handle);
	}

	void mailboxPush(const T& item){
		static_assert(N == 1, "mailbox push only works on queues of size 1");
		xQueueOverwrite(handle, &item);
	}

	bool empty(){
		return uxQueueMessagesWaiting(handle) == 0;
	}
};
