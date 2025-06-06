// An atomic operation is an indivisible operation. You can’t observe such an operation
// half-done from any thread in the system; it’s either done or not done. If the load oper-
// ation that reads the value of an object is atomic, and all modifications to that object are
// also atomic, that load will retrieve either the initial value of the object or the value
// stored by one of the modifications.

// The flip side of this is that a nonatomic operation might be seen as half-done by
// another thread. If that operation is a store, the value observed by another thread
// might be neither the value before the store nor the value stored but something else. If
// the nonatomic operation is a load, it might retrieve part of the object, have another
// thread modify the value, and then retrieve the remainder of the object, thus retrieving
// neither the first value nor the second but some combination of the two. This is a sim-
// ple problematic race condition, as described in chapter 3, but at this level it may con-
// stitute a data race (see section 5.1) and thus cause undefined behavior.