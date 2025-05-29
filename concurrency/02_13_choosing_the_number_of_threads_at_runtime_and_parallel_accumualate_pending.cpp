// It divides the work among the threads, with a minimum number of elements per
// thread in order to avoid the overhead of too many threads. Note that this implementa-
// tion assumes that none of the operations will throw an exception, even though excep-
// tions are possible; the std::thread constructor will throw if it can’t start a new thread
// of execution, for example. Handling exceptions in such an algorithm is beyond the
// scope of this simple example and will be covered in chapter 8.
// Listing 2.8 A naïve parallel version of std::accumulate



/*

template<typename Iterator,typename T>
struct accumulate_block
{
    void operator()(Iterator first,Iterator last,T& result)
    {
        result=std::accumulate(first,last,result);
    }
};
    
template<typename Iterator,typename T>
T parallel_accumulate(Iterator first,Iterator last,T init)
{
    unsigned long const length=std::distance(first,last);

    if(!length)
        return init;
    
    unsigned long const min_per_thread=25;
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;
    unsigned long const hardware_threads=std::thread::hardware_concurrency();
    unsigned long const num_threads=std::min(hardware_threads!=0?hardware_threads:2,max_threads);
    unsigned long const block_size=length/num_threads;

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads-1);f
    
    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);++i)
    {
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        threads[i]=std::thread(
        accumulate_block<Iterator,T>(),
        block_start,block_end,std::ref(results[i]));
        block_start=block_end;
    }

    accumulate_block<Iterator,T>()(
    block_start,last,results[num_threads-1]);
    
    std::for_each(threads.begin(),threads.end(), std::mem_fn(&std::thread::join));

    return std::accumulate(results.begin(),results.end(),init);
}

*/



// Although this is quite a long function, it’s actually straightforward. If the input range
// is empty B, you just return the initial value init. Otherwise, there’s at least one ele-
// ment in the range, so you can divide the number of elements to process by the mini-
// mum block size in order to give the maximum number of threads c. This is to avoid
// creating 32 threads on a 32-core machine when you have only five values in the range.
// The number of threads to run is the minimum of your calculated maximum and
// the number of hardware threads d. You don’t want to run more threads than the
// hardware can support (which is called oversubscription), because the context switching
// will mean that more threads will decrease the performance. If the call to std::thread::
// hardware_concurrency() returned 0, you’d simply substitute a number of your choice;
// in this case I’ve chosen 2. You don’t want to run too many threads, because that would
// slow things down on a single-core machine, but likewise you don’t want to run too few,
// because then you’d be passing up the available concurrency.
// The number of entries for each thread to process is the length of the range
// divided by the number of threads e. If you’re worrying about the case where the
// number doesn’t divide evenly, don’t—you’ll handle that later.
// Now that you know how many threads you have, you can create a std::vector<T>
// for the intermediate results and a std::vector<std::thread> for the threads f.
// Note that you need to launch one fewer thread than num_threads, because you already
// have one.
// Launching the threads is just a simple loop: advance the block_end iterator to the
// end of the current block g and launch a new thread to accumulate the results for this
// block h. The start of the next block is the end of this one i.
// After you’ve launched all the threads, this thread can then process the final block
// .
// j This is where you take account of any uneven division: you know the end of the
// final block must be last, and it doesn’t matter how many elements are in that block.
// Once you’ve accumulated the results for the last block, you can wait for all the
// threads you spawned with std::for_each 1), as in listing 2.7, and then add up the results
// with a final call to std::accumulate 1!.
// Before you leave this example, it’s worth pointing out that where the addition
// operator for the type T is not associative (such as for float or double), the results of
// this parallel_accumulate may vary from those of std::accumulate, because of the
// grouping of the range into blocks. Also, the requirements on the iterators are slightly
// more stringent: they must be at least forward iterators, whereas std::accumulate can
// work with single-pass input iterators, and T must be default constructible so that you can cre-
// ate the results vector. These sorts of requirement changes are common with parallel
// algorithms; by their very nature they’re different in some manner in order to make
// them parallel, and this has consequences on the results and requirements. Parallel
// algorithms are covered in more depth in chapter 8. It’s also worth noting that because
// you can’t return a value directly from a thread, you must pass in a reference to the rel-
// evant entry in the results vector. Alternative ways of returning results from threads
// are addressed through the use of futures in chapter 4.
// In this case, all the information required by each thread was passed in when the
// thread was started, including the location in which to store the result of its calculation.
// This isn’t always the case: sometimes it’s necessary to be able to identify the threads in
// some way for part of the processing. You could pass in an identifying number, such as
// the value of i in listing 2.7, but if the function that needs the identifier is several levels
// deep in the call stack and could be called from any thread, it’s inconvenient to have to
// do it that way. When we were designing the C++ Thread Library we foresaw this need,
// and so each thread has a unique identifier.