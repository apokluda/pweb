/* 
 * File:   OverlayID.h
 * Author: mfbari
 *
 * Created on November 26, 2012, 1:10 AM
 */

#ifndef OVERLAY_ID_H
#define	OVERLAY_ID_H

#include <iostream>
#include <cmath>

#include "GlobalData.h"
#include "../protocol/code.h"
//#include "../../common/util.h"

using namespace std;

//class GlobalData;

class OverlayID {
        long overlay_id;
        int prefix_length;
public:
        int MAX_LENGTH;

        void INIT() {
                //ReedMuller *rm = new ReedMuller(2, 4);
                //MAX_LENGTH = rm->rm->k;
                //MAX_LENGTH = GlobalData::rm->rm->k;
        }

        OverlayID() {
        }

        OverlayID(long overlay_id, int prefix_lenght, ABSCode *iCode) {
                this->overlay_id = overlay_id;
                this->prefix_length = prefix_lenght;
                MAX_LENGTH = iCode->K();
        }

        OverlayID(long overlay_id, int prefix_lenght, int max_length) {
                this->overlay_id = overlay_id;
                this->prefix_length = prefix_lenght;
                MAX_LENGTH = max_length;
        }

        OverlayID(long pattern, ABSCode *iCode) {
                this->overlay_id = iCode->decode(pattern);
                this->prefix_length = iCode->K();
                MAX_LENGTH = iCode->K();
        }

        void SetPrefix_length(int prefix_length) {
                this->prefix_length = prefix_length;
        }

        int GetPrefix_length() const {
                return prefix_length;
        }

        void SetOverlay_id(int overlay_id) {
                this->overlay_id = overlay_id;
        }

        int GetOverlay_id() const {
                return overlay_id;
        }

        int GetBitAtPosition(int n) const {
                int value = this->overlay_id;
                return (((value & (1 << n)) >> n) & 0x00000001);
        }

        OverlayID ToggleBitAtPosition(int n) const {
                OverlayID id(this->overlay_id, this->prefix_length, this->MAX_LENGTH);
                id.overlay_id ^= (1 << n);
                return id;
        }

        int GetMatchedPrefixLength(const OverlayID &id) const {
                int position = MAX_LENGTH - 1;
                for (; position > 0; position--) {
                        if (this->GetBitAtPosition(position)
                                != id.GetBitAtPosition(position)) {
                                break;
                        }
                }
                return MAX_LENGTH - position - 1;
        }

        bool operator==(const OverlayID &id) const {
                if (this->prefix_length != id.prefix_length)
                        return false;
                if (this->overlay_id != id.overlay_id)
                        return false;
                return true;
        }

        bool operator!=(const OverlayID &id) const {
                return !(*this == id);
        }

        //    bool operator<=(const OverlayID &id) {
        //        return this->overlay_id <= id.overlay_id;
        //    }

        bool operator<(const OverlayID &id) const {
                return this->overlay_id < id.overlay_id;
        }

        void printBits() {
        		printf(">>>MAX_LENGTH = %d<<<", MAX_LENGTH);
                for (int i = MAX_LENGTH - 1; i >= 0; i--) {
                        cout << GetBitAtPosition(i);
                }
        }

        int getBitAtPosition(int value, int n) {
                return (((value & (1 << n)) >> n) & 0x00000001);
        }

        char* toString() {
                char* result = new char[getStringSize() + 1];
                for (int i = MAX_LENGTH - 1; i >= 0; i--) {
                        result[MAX_LENGTH - i - 1] = '0' + getBitAtPosition(overlay_id, i);
                }
                sprintf(result + MAX_LENGTH, ":%d", prefix_length);
                result[getStringSize()] = '\0';
                return result;
        }

        unsigned numDigits(unsigned number) {
                int digits = 0;
                while (number != 0) {
                        number /= 10;
                        digits++;
                }
                return digits;
        }

        int getStringSize() {
                return MAX_LENGTH + 1 + numDigits(prefix_length);
        }
};
#endif	/* OVERLAY_ID_H */

