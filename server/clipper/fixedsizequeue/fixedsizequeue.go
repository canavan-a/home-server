package fixedsizequeue

type FixedQueue[T any] struct {
	size int
	data []T // this is actually in reverse order
}

func CreateFixedQueue[T any](size int) *FixedQueue[T] {
	return &FixedQueue[T]{
		size: size,
		data: make([]T, size),
	}
}

func (fq *FixedQueue[T]) Add(value T) {
	for i := range fq.data {
		if i == fq.size-1 {
			break
		}
		fq.data[i] = fq.data[i+1]
	}

	fq.data[fq.size-1] = value
}

func (fq *FixedQueue[T]) CopyOut() []T {

	dataCopy := make([]T, fq.size)

	copy(dataCopy, fq.data)

	return dataCopy
}
