// Timer.hpp

//          
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef _LMC_TIMER_HPP_
#define _LMC_TIMER_HPP_

#include <string>
#include <boost/timer.hpp>

namespace timer{
	class Timer	{
		boost::timer t_;
		double _seconds;
	public:
		Timer(double length):_seconds(length){}
		virtual ~Timer() {}
		bool poll(){ 
			bool done = (t_.elapsed() > _seconds);
			if(done){
        Done();
      }
			return done; 
		}
		double elapsed(){
      return t_.elapsed();
    }
		virtual void Done() = 0;
	};
}
#endif