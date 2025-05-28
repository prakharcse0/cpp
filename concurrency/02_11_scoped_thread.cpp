// One benefit of the move support of std::thread is that you can build on the thread_guard class from earlier
// and have it actually take ownership of the thread.
// This avoids any unpleasant consequences should the thread_guard object outlive the thread it was referencing, 
// and it also means that no one else can join or detach the thread once ownership has been transferred into the object. 
// Because this would primarily be aimed at ensuring threads are completed before a scope is exited, I named this class scoped_thread. 
