/*
 * code.h
 *
 *  Created on: 2012-11-21
 *      Author: sr2chowd
 */

#ifndef NULL_CODE_H_
#define NULL_CODE_H_

class NullCode : public ABSCode {
	int bit_length;
public:

        NullCode(int bit_length) : ABSCode(){
		this->bit_length = bit_length;
        };

        virtual ~NullCode() {
        };

        virtual long encode(long message) {
                return message;
        }

        virtual long decode(long codeword) {
                return codeword;
        }

        virtual int K() {
                return bit_length;
        };

        virtual int N() {
                return bit_length;
        };
};

#endif /* ICODE_H_ */
