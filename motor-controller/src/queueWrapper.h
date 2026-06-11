#pragma once

#include <stdint.h>

template<typename T, int N>
struct Queue {
	T buffer[N];
	uint8_t head{0}, tail{0}, count{0};

	void send(const T& item){
		buffer[head] = item;
		head = (head + 1) % N;
		if(count < N) count++;
	}

	bool receiveNonBlocking(T& out){
		if(count == 0) return false;
		out = buffer[tail];
		tail = (tail + 1) % N;
		count--;
		return true;
	}
};
