#ifndef GOLEY_CODE_H_
#define GOLEY_CODE_H_

#include "../../code.h"
#include "golay.h"

class GolayCode : public ABSCode {
public:

        GolayCode() : ABSCode() {
        };

        long encode(long message) {
                return encode_golay(message);
        }

        long decode(long codeword) {
                return decode_golay(codeword);
        }

        int K() {
                return 12;
        }

        int N() {
                return 23;
        }
};

#endif
