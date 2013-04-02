/*
 * code.h
 *
 *  Created on: 2012-11-21
 *      Author: sr2chowd
 */

#ifndef CODE_H_
#define CODE_H_

class ABSCode {
public:

        ABSCode() {
        };

        virtual ~ABSCode() {
        };

        virtual long encode(long message)=0;

        virtual long decode(long codeword)=0;

        virtual int K()=0;

        virtual int N()=0;
};

#endif /* CODE_H_ */
