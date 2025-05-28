// You detach a thread by calling the detach() member function of the std::thread object. 

// After the call completes, the std::thread object is no longer associated
// with the actual thread of execution and is therefore no longer joinable:

// std::thread t(do_background_work);
// t.detach();
// assert(!t.joinable());

// In order to detach the thread from a std::thread object, there must be a thread to detach: 
// you can’t call detach() on a std::thread object with no associated thread of execution. 
// This is exactly the same requirement as for join(), 
// and you can check it in exactly the same way — 
// you can only call t.detach() for a std::thread object t when t.joinable() returns true.


// Consider an application such as a word processor that can edit multiple documents at once. 
// There are many ways to handle this, both at the UI level and internally.
// One way that seems to be increasingly common at the moment is to have multiple
// independent top-level windows, one for each document being edited.

void edit_document(std::string const& filename)
{
    open_document_and_display_gui(filename);
    while(!done_editing())
    {
        user_command cmd=get_user_input();
        if(cmd.type==open_new_document) {
            std::string const new_name=get_filename_from_user();
            std::thread t(edit_document,new_name);
            t.detach();
        }
        else {
            process_user_input(cmd);
        }
    }
}