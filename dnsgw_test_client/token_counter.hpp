/*
 *  Copyright (C) 2013 Alexander Pokluda
 *
 *  This file is part of pWeb.
 *
 *  pWeb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pWeb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pWeb.  If not, see <http://www.gnu.org/licenses/>.
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
