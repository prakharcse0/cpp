/*
===============================================================================
                        std::promise COMPREHENSIVE NOTES
===============================================================================

SUMMARY:
- std::promise<T> sets values that std::future<T> can later read
- Enables cross-thread communication without blocking sender
- Alternative to thread-per-connection for high-concurrency network apps
- Sender uses promise.set_value(), receiver blocks on future.get()
- Exception handling: destroying promise without setting value stores exception

KEY CONCEPTS:
- Promise/Future pair: One-time communication channel between threads
- Network scalability: Single thread handles multiple connections
- Asynchronous data flow: Random packet arrival/sending with coordinated responses

WORKFLOW:
1. Create promise, get associated future via get_future()
2. Pass future to waiting thread, promise to data-providing thread
3. Sender calls set_value() → future becomes ready
4. Receiver unblocks and retrieves value
===============================================================================
*/

#include <future>
#include <vector>
#include <queue>

// Forward declarations and type definitions for the example
struct payload_type { /* network data */ };
struct data_packet { 
    int id; 
    payload_type payload; 
};
struct outgoing_packet { 
    payload_type payload; 
    std::promise<bool> promise; 
};

class connection {
public:
    bool has_incoming_data() const { return true; /* simplified */ }
    bool has_outgoing_data() const { return true; /* simplified */ }
    data_packet incoming() { return {}; /* simplified */ }
    outgoing_packet top_of_outgoing_queue() { return {}; /* simplified */ }
    void send(const payload_type& data) { /* network send */ }
    std::promise<payload_type>& get_promise(int id) { 
        static std::promise<payload_type> p; 
        return p; /* simplified lookup */ 
    }
};

using connection_set = std::vector<connection>;
using connection_iterator = connection_set::iterator;

bool done(const connection_set& connections) { 
    return false; /* simplified - would check if all connections closed */ 
}

// MAIN EXAMPLE: Single thread handling multiple network connections
void process_connections(connection_set& connections)
{
    // Main event loop - continues until all connections done
    while(!done(connections))                                    // B
    {
        // Iterate through all active connections
        for(connection_iterator 
            connection=connections.begin(),end=connections.end();
            connection!=end;
            ++connection)                                        // C
        {
            // INCOMING DATA HANDLING
            if(connection->has_incoming_data())                  // D
            {
                // Extract packet with ID and payload
                data_packet data=connection->incoming();
                
                // Map packet ID to corresponding promise
                // (typically via associative container lookup)
                std::promise<payload_type>& p=
                    connection->get_promise(data.id);            // E
                
                // Fulfill promise - waiting future becomes ready
                p.set_value(data.payload);
            }
            
            // OUTGOING DATA HANDLING  
            if(connection->has_outgoing_data())                  // F
            {
                // Get next packet from outgoing queue
                outgoing_packet data=
                    connection->top_of_outgoing_queue();
                
                // Actually send data through network connection
                connection->send(data.payload);
                
                // Signal successful transmission via promise
                data.promise.set_value(true);                    // G
            }
        }
    }
}

/*
PROMISE/FUTURE MECHANICS:
- std::promise<T>: Write-once value setter
- std::future<T>: Read-once value getter  
- get_future(): Links promise to its corresponding future
- set_value(val): Makes future ready with value
- Destruction without set_value(): Stores exception in future

SCALING BENEFITS:
- Avoids thread-per-connection (resource intensive)
- Reduces context switching overhead
- Handles high connection counts efficiently
- OS resources preserved for actual network capacity

ERROR HANDLING:
- Exceptions can be stored in promise/future
- Real-world scenarios: disk full, network failure, DB down
- C++ Standard Library provides clean exception propagation
- Alternative to restrictive success-only requirement

TYPICAL USE CASES:
- Network servers with many concurrent connections
- Asynchronous I/O operations
- Cross-thread result coordination
- Event-driven architectures

===============================================================================
                    std::packaged_task vs std::promise
===============================================================================

CONCEPTUAL DIFFERENCES:

std::packaged_task:
- Wraps a CALLABLE (function/lambda/functor)
- Automatically executes function and captures result in future
- "Function-centric" - you have code to run
- get_future() then task() - execution happens when you call the task
- Exception handling: automatically catches and stores function exceptions

std::promise:
- Manual value/exception setting mechanism
- You explicitly set_value() or set_exception() 
- "Data-centric" - you have a value to communicate
- get_future() then set_value() - you control when future becomes ready
- Exception handling: you decide what constitutes an error

WHEN TO USE WHICH:

Use packaged_task when:
- You have a function to execute on another thread
- Want automatic exception handling
- Function execution = result availability
- Example: Running computation, file I/O operation

Use promise when:
- You manually determine when result is ready
- Result comes from external events (network, user input, timers)
- Need fine-grained control over success/failure conditions  
- Example: Network responses, GUI events, sensor data

MENTAL MODEL:
- packaged_task = "Run this function elsewhere, tell me when done"
- promise = "I'll tell you when I have your answer"
*/