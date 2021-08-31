#include "include/LogDuration.hpp"

#include <typeinfo>

LogDuration::LogDuration(const std::string& id) 
    : id_(id), out_(std::cerr)
{
}

LogDuration::LogDuration(const std::string& id, std::ostream& out) 
    : id_(id), out_(out)
{
}

LogDuration::~LogDuration()
{
    using namespace std;
    using namespace chrono;
    using namespace literals;

    const auto end_time = Clock::now();
    const auto dur = end_time - start_time_;
    
    const std::string output_thread_name = typeid(out_).name();
    
    std::string output_prefix = typeid(out_).name();

    // TODO: how can I determine the type of out_?  For example it would be std::cout or std::cerr
    //std::cout << "---PREFIX---: " << typeid(out_).name() << std::endl;

    // TODO: rewrite after debugging with "() ? () : ()" style
    if (output_thread_name == "std::cout"s)     // ... how (first TODO)?
    {
        output_prefix = "Operation time: "s;
    }
    else
    {
        output_prefix = id_ + ": "s;
    }

    out_ << output_prefix << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
}