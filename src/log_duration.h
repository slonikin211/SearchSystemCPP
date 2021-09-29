#pragma once

#include <chrono>
#include <iostream>
#include <typeinfo>

class LogDuration 
{
public:
    // For better usage and code beauty
    using Clock = std::chrono::steady_clock;

    // Declarations //
    LogDuration(const std::string& id);
    LogDuration(const std::string& id, std::ostream& out);
    ~LogDuration();
private:
    const std::string id_;
    std::ostream& out_;
    const Clock::time_point start_time_ = Clock::now();
};

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, out) LogDuration UNIQUE_VAR_NAME_PROFILE(x, out)


// Definitions //

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
    // if (output_thread_name == "std::cout"s)     // ... how (first TODO)?
    // {
    //     output_prefix = "Operation time: "s;
    // }
    // else
    // {
    //     output_prefix = id_ + ": "s;
    // }

    output_prefix = (output_thread_name == "std::cout"s) ? ("Operation time: "s) : (id_ + ": "s);

    out_ << output_prefix << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
}