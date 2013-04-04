/*
 * token_counter.hpp
 *
 *  Created on: 2013-04-03
 *      Author: apokluda
 */

#ifndef TOKEN_COUNTER_HPP_
#define TOKEN_COUNTER_HPP_

class token_counter
{
public:
    typedef boost::function< void() > cb_t;

    token_counter(std::size_t const numtokens)
    : numtokens_(numtokens)
    {
    }

    std::size_t request(std::size_t num)
    {
        if ( num > numtokens_ ) num = numtokens_;
        numtokens_ -= num;
        return num;
    }

    void release(std::size_t num)
    {
        numtokens_ += num;
        notify();
    }

    void wait(cb_t cb) const
    {
        cbqueue_.push(cb);
    }

private:
    void notify()
    {
        while ( numtokens_ && !cbqueue_.empty() )
        {
            cbqueue_.front()();
            cbqueue_.pop();
        }
    }

    mutable std::queue< cb_t > cbqueue_;
    std::size_t numtokens_;
};

#endif /* TOKEN_COUNTER_HPP_ */
