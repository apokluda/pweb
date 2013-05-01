/*
 * service.hpp
 *
 *  Created on: 2013-05-01
 *      Author: apokluda
 */

#ifndef SERVICE_HPP_
#define SERVICE_HPP_

void checked_io_service_run(boost::asio::io_service& io_service);
void run( boost::asio::io_service& io_service, std::size_t const num_threads );

#endif /* SERVICE_HPP_ */
