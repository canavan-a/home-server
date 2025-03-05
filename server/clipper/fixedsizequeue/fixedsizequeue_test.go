package fixedsizequeue_test

import (
	"fmt"
	"main/clipper/fixedsizequeue"
	"testing"
)

func TestFSQ(t *testing.T) {
	myQ := fixedsizequeue.CreateFixedQueue[int](10)

	myQ.Add(4)
	fmt.Println(myQ.CopyOut())
	myQ.Add(3)
	fmt.Println(myQ.CopyOut())
	myQ.Add(2)
	fmt.Println(myQ.CopyOut())
	myQ.Add(1)
	fmt.Println(myQ.CopyOut())
	myQ.Add(2)
	fmt.Println(myQ.CopyOut())
	myQ.Add(3)
	fmt.Println(myQ.CopyOut())
	myQ.Add(4)
	fmt.Println(myQ.CopyOut())
	myQ.Add(5)
	fmt.Println(myQ.CopyOut())
	myQ.Add(6)
	fmt.Println(myQ.CopyOut())
	myQ.Add(7)
	fmt.Println(myQ.CopyOut())
	myQ.Add(8)
	fmt.Println(myQ.CopyOut())
	myQ.Add(9)

	value := myQ.CopyOut()

	fmt.Println(value)
}
