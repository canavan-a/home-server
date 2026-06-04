#pragma once

#include <Arduino_FreeRTOS.h>

template<typename T>
struct Queue{
	QueueHandle_t handle;

	Queue(int size){
		handle = xQueueCreate(size, sizeof(T));
	}

	void send(const T& item){
		xQueueSend(handle, &item, portMAX_DELAY);
	}

	T receive(){
		T item;
		xQueueReceive(handle, &item, portMAX_DELAY);
		return item;
	}

	T receiveUntil(T& out, TickType_t tickValue){
		return xQueueReceive(handle, &out, tickValue) == pdTRUE;
	}

	void clear(){
		xQueueReset(handle);
	}

	void mailboxPush(const T& item){
		static_assert(Size == 1, "mailbox push only works on queues of size 1");
		xQueueOverwrite(handle, &item);
	}

	bool empty(){
		return uxQueueMessagesWaiting(handle) == 0;
	}
};
